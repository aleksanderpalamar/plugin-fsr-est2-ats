#include "report.hpp"
#include "vulkan_device.hpp"

#include <vk/ffx_api_vk.h>
#include <ffx_upscale.h>
#include <cstdio>
#include <string>
#include <vector>

namespace {
using namespace spike;

struct Api {
    PfnFfxCreateContext create = nullptr;
    PfnFfxDestroyContext destroy = nullptr;
    PfnFfxQuery query = nullptr;
};

void message(uint32_t type, const wchar_t* text) {
    std::printf("fsr.message type=%u text=%s\n", type, narrow(text).c_str());
}

std::optional<Api> load_api() {
    HMODULE module = LoadLibraryW(L"amd_fidelityfx_loader_vk.dll");
    if (!report("fsr.vk.loader", module ? S_OK : HRESULT_FROM_WIN32(GetLastError()))) return std::nullopt;
    Api api{reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(module, "ffxCreateContext")),
        reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(module, "ffxDestroyContext")),
        reinterpret_cast<PfnFfxQuery>(GetProcAddress(module, "ffxQuery"))};
    if (!report("fsr.vk.exports", api.create && api.destroy && api.query ? S_OK : E_NOINTERFACE)) return std::nullopt;
    return api;
}

VkDeviceContext device_context(const VulkanDevice& vulkan) {
    return {vulkan.device, vulkan.physical, vulkan.device_proc};
}

std::vector<std::pair<uint64_t, std::string>> versions(const Api& api, VkDeviceContext& context) {
    uint64_t count = 0;
    ffxQueryDescGetVersions query{};
    query.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
    query.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
    query.device = &context;
    query.outputCount = &count;
    auto first = api.query(nullptr, &query.header);
    std::vector<uint64_t> ids(count);
    std::vector<const char*> names(count);
    query.versionIds = ids.data();
    query.versionNames = names.data();
    auto second = count > 0 ? api.query(nullptr, &query.header) : first;
    report("fsr.vk.versions", first == FFX_API_RETURN_OK && second == FFX_API_RETURN_OK ? S_OK : E_FAIL,
        "count=" + std::to_string(count) + " code=" + std::to_string(second));
    std::vector<std::pair<uint64_t, std::string>> output;
    for (uint64_t index = 0; index < count; ++index) output.emplace_back(ids[index], names[index] ? names[index] : "");
    return output;
}

void try_version(const Api& api, const VulkanDevice& vulkan, uint64_t id, const std::string& name) {
    std::printf("fsr.vk.try id=0x%llx name=%s\n", static_cast<unsigned long long>(id), name.c_str());
    ffxCreateBackendVKDesc backend{};
    backend.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK;
    backend.deviceContext = device_context(vulkan);
    ffxOverrideVersion override_version{};
    override_version.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
    override_version.versionId = id;
    backend.header.pNext = &override_version.header;
    ffxCreateContextDescUpscale create{};
    create.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
    create.header.pNext = &backend.header;
    create.flags = FFX_UPSCALE_ENABLE_AUTO_EXPOSURE | FFX_UPSCALE_ENABLE_DEBUG_CHECKING;
    create.maxRenderSize = {640, 360};
    create.maxUpscaleSize = {1280, 720};
    create.fpMessage = message;
    ffxContext context = nullptr;
    auto code = api.create(&context, &create.header, nullptr);
    if (!report("fsr.vk.context", code == FFX_API_RETURN_OK ? S_OK : E_FAIL, "code=" + std::to_string(code))) return;
    ffxQueryGetProviderVersion provider{};
    provider.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;
    code = api.query(&context, &provider.header);
    char text[32]{};
    std::snprintf(text, sizeof(text), "0x%llx", static_cast<unsigned long long>(provider.versionId));
    report("fsr.vk.provider", code == FFX_API_RETURN_OK ? S_OK : E_FAIL,
        std::string("id=") + text + " name=" + (provider.versionName ? provider.versionName : "null"));
    api.destroy(&context, nullptr);
}
}

int main() {
    if (!std::freopen("fsr4_vulkan_probe.log", "w", stdout)) return 4;
    auto vulkan = create_vulkan_device();
    if (!vulkan) return 1;
    auto api = load_api();
    if (!api) return 2;
    auto context = device_context(*vulkan);
    auto available = versions(*api, context);
    for (const auto& [id, name] : available) try_version(*api, *vulkan, id, name);
    std::printf("summary vk_versions=%zu\n", available.size());
    return available.empty() ? 3 : 0;
}
