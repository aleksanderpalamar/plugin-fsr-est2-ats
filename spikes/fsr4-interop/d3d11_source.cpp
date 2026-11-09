#include "d3d11_source.hpp"
#include "report.hpp"

#include <dxgi1_2.h>
#include <string>

namespace spike {
namespace {
bool describe_adapter(D3D11Source& source) {
    ComPtr<IDXGIDevice> device;
    ComPtr<IDXGIAdapter> adapter;
    HRESULT result = source.device.As(&device);
    if (SUCCEEDED(result)) result = device->GetAdapter(&adapter);
    DXGI_ADAPTER_DESC description{};
    if (SUCCEEDED(result)) result = adapter->GetDesc(&description);
    source.adapter = description.AdapterLuid;
    return report("d3d11.adapter", result, "name=" + narrow(description.Description) +
        " vendor=0x" + std::to_string(description.VendorId));
}

bool create_fence(D3D11Source& source) {
    HRESULT result = source.device->CreateFence(0, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(&source.fence));
    if (!report("d3d11.fence.create", result)) return false;
    result = source.fence->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, &source.fence_handle);
    return report("d3d11.fence.share", result);
}
}

std::optional<D3D11Source> create_source() {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL levels[]{D3D_FEATURE_LEVEL_11_1};
    D3D_FEATURE_LEVEL level{};
    HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 1,
        D3D11_SDK_VERSION, &device, &level, &context);
    if (!report("d3d11.device", result, "feature_level=0x" + std::to_string(level))) return std::nullopt;
    D3D11Source source;
    if (!report("d3d11.device5", device.As(&source.device))) return std::nullopt;
    if (!report("d3d11.context4", context.As(&source.context))) return std::nullopt;
    if (!describe_adapter(source) || !create_fence(source)) return std::nullopt;
    return source;
}

std::optional<SharedTexture> create_shared(D3D11Source& source, const char* name, DXGI_FORMAT format,
    UINT width, UINT height, const std::vector<unsigned char>& pixels, UINT pitch) {
    D3D11_TEXTURE2D_DESC description{};
    description.Width = width;
    description.Height = height;
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = format;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    description.MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
    D3D11_SUBRESOURCE_DATA data{pixels.data(), pitch, 0};
    SharedTexture shared{nullptr, nullptr, format, width, height};
    std::string step = std::string("d3d11.texture.") + name;
    HRESULT result = source.device->CreateTexture2D(&description, &data, &shared.texture);
    if (!report(step.c_str(), result)) return std::nullopt;
    ComPtr<IDXGIResource1> resource;
    result = shared.texture.As(&resource);
    if (SUCCEEDED(result))
        result = resource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE,
            nullptr, &shared.handle);
    if (!report((step + ".share").c_str(), result)) return std::nullopt;
    return shared;
}

UINT64 signal(D3D11Source& source) {
    ++source.fence_value;
    source.context->Signal(source.fence.Get(), source.fence_value);
    source.context->Flush();
    return source.fence_value;
}
}
