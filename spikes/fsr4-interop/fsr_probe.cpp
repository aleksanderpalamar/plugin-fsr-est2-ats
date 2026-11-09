#include "fsr_probe.hpp"
#include "d3d12_readback.hpp"
#include "report.hpp"

#include <dx12/ffx_api_dx12.hpp>
#include <ffx_upscale.h>
#include <algorithm>
#include <string>
#include <vector>

namespace spike {
namespace {
struct Api {
    PfnFfxCreateContext create = nullptr;
    PfnFfxDestroyContext destroy = nullptr;
    PfnFfxQuery query = nullptr;
    PfnFfxDispatch dispatch = nullptr;
};

struct Version {
    uint64_t id = 0;
    std::string name;
};

void message(uint32_t type, const wchar_t* text) {
    std::printf("fsr.message type=%u text=%s\n", type, narrow(text).c_str());
}

std::optional<Api> load_api() {
    HMODULE module = LoadLibraryW(L"amd_fidelityfx_loader_dx12.dll");
    if (!report("fsr.loader", module ? S_OK : HRESULT_FROM_WIN32(GetLastError()))) return std::nullopt;
    Api api{reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(module, "ffxCreateContext")),
        reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(module, "ffxDestroyContext")),
        reinterpret_cast<PfnFfxQuery>(GetProcAddress(module, "ffxQuery")),
        reinterpret_cast<PfnFfxDispatch>(GetProcAddress(module, "ffxDispatch"))};
    bool complete = api.create && api.destroy && api.query && api.dispatch;
    if (!report("fsr.exports", complete ? S_OK : E_NOINTERFACE)) return std::nullopt;
    return api;
}

std::vector<Version> versions(const Api& api, ID3D12Device* device) {
    uint64_t count = 0;
    ffxQueryDescGetVersions query{};
    query.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
    query.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
    query.device = device;
    query.outputCount = &count;
    auto first = api.query(nullptr, &query.header);
    std::vector<uint64_t> ids(count);
    std::vector<const char*> names(count);
    query.versionIds = ids.data();
    query.versionNames = names.data();
    auto second = count > 0 ? api.query(nullptr, &query.header) : first;
    report("fsr.versions", first == FFX_API_RETURN_OK && second == FFX_API_RETURN_OK ? S_OK : E_FAIL,
        "count=" + std::to_string(count) + " code=" + std::to_string(second));
    std::vector<Version> output;
    for (uint64_t index = 0; index < count; ++index) output.push_back({ids[index], names[index] ? names[index] : ""});
    return output;
}

ComPtr<ID3D12Resource> create_output(ID3D12Device* device, UINT width, UINT height) {
    D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0};
    D3D12_RESOURCE_DESC description{D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, width, height, 1, 1,
        DXGI_FORMAT_R16G16B16A16_FLOAT, {1, 0}, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS};
    ComPtr<ID3D12Resource> output;
    device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &description,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&output));
    return output;
}

ffxDispatchDescUpscale dispatch_description(D3D12Sink& sink, const FsrInputs& inputs, ID3D12Resource* output,
    bool reset) {
    ffxDispatchDescUpscale dispatch{};
    dispatch.header.type = FFX_API_DISPATCH_DESC_TYPE_UPSCALE;
    dispatch.commandList = sink.list.Get();
    dispatch.color = ffxApiGetResourceDX12(inputs.color, FFX_API_RESOURCE_STATE_COMMON);
    dispatch.depth = ffxApiGetResourceDX12(inputs.depth, FFX_API_RESOURCE_STATE_COMMON);
    dispatch.motionVectors = ffxApiGetResourceDX12(inputs.motion, FFX_API_RESOURCE_STATE_COMMON);
    dispatch.output = ffxApiGetResourceDX12(output, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
    dispatch.motionVectorScale = {1.0f, 1.0f};
    dispatch.renderSize = {inputs.width, inputs.height};
    dispatch.upscaleSize = {inputs.width * 2, inputs.height * 2};
    dispatch.frameTimeDelta = 16.6f;
    dispatch.preExposure = 1.0f;
    dispatch.reset = reset;
    dispatch.cameraNear = 0.1f;
    dispatch.cameraFar = 1000.0f;
    dispatch.cameraFovAngleVertical = 1.0f;
    dispatch.viewSpaceToMetersFactor = 1.0f;
    return dispatch;
}

bool run_frames(const Api& api, ffxContext& context, D3D12Sink& sink, const FsrInputs& inputs,
    ID3D12Resource* output, UINT64 wait_value) {
    for (int frame = 0; frame < 4; ++frame) {
        auto dispatch = dispatch_description(sink, inputs, output, frame == 0);
        begin_timing(sink);
        auto code = api.dispatch(&context, &dispatch.header);
        end_timing(sink);
        bool submitted = submit(sink, frame == 0 ? wait_value : 0);
        std::string step = "fsr.dispatch frame=" + std::to_string(frame);
        if (!report(step.c_str(), code == FFX_API_RETURN_OK && submitted ? S_OK : E_FAIL,
            "code=" + std::to_string(code) + " gpu_ms=" + std::to_string(elapsed_ms(sink)))) return false;
    }
    return true;
}

void check_output(D3D12Sink& sink, ID3D12Resource* output) {
    auto readback = read_texture(sink, output, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0);
    if (!report("fsr.output.read", readback ? S_OK : E_FAIL)) return;
    auto nonzero = std::count_if(readback->pixels.begin(), readback->pixels.end(), [](unsigned char v) { return v != 0; });
    report("fsr.output.content", nonzero > 0 ? S_OK : E_FAIL, "nonzero_bytes=" + std::to_string(nonzero));
}

void try_version(const Api& api, D3D12Sink& sink, const FsrInputs& inputs, const Version& version, UINT64 wait_value) {
    std::printf("fsr.try id=0x%llx name=%s\n", static_cast<unsigned long long>(version.id), version.name.c_str());
    ffxCreateBackendDX12Desc backend{};
    backend.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
    backend.device = sink.device.Get();
    ffxOverrideVersion override_version{};
    override_version.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
    override_version.versionId = version.id;
    backend.header.pNext = &override_version.header;
    ffxCreateContextDescUpscale create{};
    create.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
    create.header.pNext = &backend.header;
    create.flags = FFX_UPSCALE_ENABLE_AUTO_EXPOSURE | FFX_UPSCALE_ENABLE_DEBUG_CHECKING;
    create.maxRenderSize = {inputs.width, inputs.height};
    create.maxUpscaleSize = {inputs.width * 2, inputs.height * 2};
    create.fpMessage = message;
    ffxContext context = nullptr;
    auto code = api.create(&context, &create.header, nullptr);
    if (!report("fsr.context", code == FFX_API_RETURN_OK ? S_OK : E_FAIL, "code=" + std::to_string(code))) return;
    ffxQueryGetProviderVersion provider{};
    provider.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;
    code = api.query(&context, &provider.header);
    char id[32]{};
    std::snprintf(id, sizeof(id), "0x%llx", static_cast<unsigned long long>(provider.versionId));
    report("fsr.provider", code == FFX_API_RETURN_OK ? S_OK : E_FAIL, std::string("id=") + id +
        " name=" + std::string(provider.versionName ? provider.versionName : "null"));
    auto output = create_output(sink.device.Get(), inputs.width * 2, inputs.height * 2);
    if (output && run_frames(api, context, sink, inputs, output.Get(), wait_value)) check_output(sink, output.Get());
    api.destroy(&context, nullptr);
}
}

bool probe_fsr(D3D12Sink& sink, const FsrInputs& inputs, UINT64 wait_value) {
    auto api = load_api();
    if (!api) return false;
    auto available = versions(*api, sink.device.Get());
    for (const auto& version : available) try_version(*api, sink, inputs, version, wait_value);
    return !available.empty();
}
}
