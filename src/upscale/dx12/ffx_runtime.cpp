#include "upscale/dx12/ffx_runtime.hpp"

#include <dx12/ffx_api_dx12.hpp>
#include <ffx_upscale.h>
#include <new>

namespace neuralfx::upscale {
struct FfxRuntime::Api {
    PfnFfxCreateContext create = nullptr;
    PfnFfxDestroyContext destroy = nullptr;
    PfnFfxQuery query = nullptr;
    PfnFfxDispatch dispatch = nullptr;
};

namespace {
constexpr const wchar_t* loader_name = L"amd_fidelityfx_loader_dx12.dll";

uint32_t create_flags(const OpenParameters& parameters) {
    uint32_t flags = FFX_UPSCALE_ENABLE_AUTO_EXPOSURE;
    if (parameters.depth_inverted) flags |= FFX_UPSCALE_ENABLE_DEPTH_INVERTED;
    if (parameters.color_non_linear) flags |= FFX_UPSCALE_ENABLE_NON_LINEAR_COLORSPACE;
    return flags;
}

FfxApiResource resource(ID3D12Resource* texture, uint32_t state) {
    return ffxApiGetResourceDX12(texture, state);
}
}

FfxRuntime::~FfxRuntime() {
    destroy();
    delete api_;
    if (module_) FreeLibrary(module_);
}

UpscaleStatus FfxRuntime::load(const std::filesystem::path& directory) noexcept {
    if (api_) return UpscaleStatus::Ready;
    module_ = LoadLibraryExW((directory / loader_name).c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!module_) return UpscaleStatus::UnavailableDependency;
    auto* api = new (std::nothrow) Api{
        reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(module_, "ffxCreateContext")),
        reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(module_, "ffxDestroyContext")),
        reinterpret_cast<PfnFfxQuery>(GetProcAddress(module_, "ffxQuery")),
        reinterpret_cast<PfnFfxDispatch>(GetProcAddress(module_, "ffxDispatch"))};
    if (!api || !api->create || !api->destroy || !api->query || !api->dispatch) {
        delete api;
        return UpscaleStatus::UnavailableDependency;
    }
    api_ = api;
    return UpscaleStatus::Ready;
}

std::vector<ProviderVersion> FfxRuntime::versions(ID3D12Device* device) {
    std::vector<ProviderVersion> found;
    if (!api_ || !device) return found;
    uint64_t count = 0;
    ffxQueryDescGetVersions query{};
    query.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
    query.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
    query.device = device;
    query.outputCount = &count;
    if (api_->query(nullptr, &query.header) != FFX_API_RETURN_OK || count == 0) return found;
    std::vector<uint64_t> ids(count);
    std::vector<const char*> names(count);
    query.versionIds = ids.data();
    query.versionNames = names.data();
    if (api_->query(nullptr, &query.header) != FFX_API_RETURN_OK) return found;
    for (uint64_t index = 0; index < count; ++index) found.push_back({ids[index], names[index] ? names[index] : ""});
    return found;
}

UpscaleStatus FfxRuntime::create(ID3D12Device* device, const ProviderVersion& provider,
    const OpenParameters& parameters) noexcept {
    if (!api_ || !device) return UpscaleStatus::UnavailableDependency;
    destroy();
    ffxCreateBackendDX12Desc backend{};
    backend.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
    backend.device = device;
    ffxOverrideVersion override_version{};
    override_version.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
    override_version.versionId = provider.id;
    backend.header.pNext = &override_version.header;
    ffxCreateContextDescUpscale description{};
    description.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
    description.header.pNext = &backend.header;
    description.flags = create_flags(parameters);
    description.maxRenderSize = {parameters.max_render.width, parameters.max_render.height};
    description.maxUpscaleSize = {parameters.max_output.width, parameters.max_output.height};
    ffxContext context = nullptr;
    auto code = api_->create(&context, &description.header, nullptr);
    if (code != FFX_API_RETURN_OK) return UpscaleStatus::BackendFailure;
    context_ = context;
    return UpscaleStatus::Ready;
}

std::string FfxRuntime::provider_name() noexcept {
    if (!api_ || !context_) return {};
    ffxContext context = static_cast<ffxContext>(context_);
    ffxQueryGetProviderVersion query{};
    query.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;
    if (api_->query(&context, &query.header) != FFX_API_RETURN_OK || !query.versionName) return {};
    return query.versionName;
}

UpscaleStatus FfxRuntime::dispatch(const FfxInputs& inputs, const FrameContract& contract) noexcept {
    if (!api_ || !context_) return UpscaleStatus::Unsupported;
    ffxContext context = static_cast<ffxContext>(context_);
    ffxDispatchDescUpscale dispatch{};
    dispatch.header.type = FFX_API_DISPATCH_DESC_TYPE_UPSCALE;
    dispatch.commandList = inputs.list;
    dispatch.color = resource(inputs.color, FFX_API_RESOURCE_STATE_COMMON);
    dispatch.depth = resource(inputs.depth, FFX_API_RESOURCE_STATE_COMMON);
    dispatch.motionVectors = resource(inputs.motion, FFX_API_RESOURCE_STATE_COMMON);
    dispatch.output = resource(inputs.output, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
    if (inputs.reactive) dispatch.reactive = resource(inputs.reactive, FFX_API_RESOURCE_STATE_COMMON);
    dispatch.jitterOffset = {contract.jitter.x, contract.jitter.y};
    dispatch.motionVectorScale = {1.0f, 1.0f};
    dispatch.renderSize = {contract.render.width, contract.render.height};
    dispatch.upscaleSize = {contract.output.width, contract.output.height};
    dispatch.frameTimeDelta = contract.frame_time_ms;
    dispatch.preExposure = 1.0f;
    dispatch.reset = contract.reset;
    dispatch.cameraNear = contract.camera.near_plane;
    dispatch.cameraFar = contract.camera.far_plane;
    dispatch.cameraFovAngleVertical = contract.camera.vertical_fov;
    dispatch.viewSpaceToMetersFactor = 1.0f;
    return api_->dispatch(&context, &dispatch.header) == FFX_API_RETURN_OK ? UpscaleStatus::Ready
                                                                          : UpscaleStatus::BackendFailure;
}

void FfxRuntime::destroy() noexcept {
    if (!api_ || !context_) return;
    ffxContext context = static_cast<ffxContext>(context_);
    api_->destroy(&context, nullptr);
    context_ = nullptr;
}
}
