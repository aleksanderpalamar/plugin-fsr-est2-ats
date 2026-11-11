#include "upscale/dx12/dx12_backend.hpp"

#include <dxgi.h>

namespace neuralfx::upscale {
namespace {
using Microsoft::WRL::ComPtr;

ComPtr<IDXGIAdapter> adapter_of(ID3D11Device* device) {
    ComPtr<IDXGIDevice> dxgi;
    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dxgi))) || FAILED(dxgi->GetAdapter(&adapter))) return nullptr;
    return adapter;
}

bool create_texture(std::optional<SharedTexture>& slot, ID3D11Device* host, ID3D12Device* device, DXGI_FORMAT format,
    temporal::RenderSize size, UINT extra_bind) {
    slot = create_shared_texture(host, device, format, size.width, size.height, extra_bind);
    return slot.has_value();
}
}

Dx12Backend::Dx12Backend(ID3D11Device* device, ID3D11DeviceContext* context, std::filesystem::path directory) noexcept
    : directory_(std::move(directory)) {
    if (device) device->QueryInterface(IID_PPV_ARGS(&device_));
    if (context) context->QueryInterface(IID_PPV_ARGS(&context_));
}

UpscaleStatus Dx12Backend::prepare() noexcept {
    if (dx12_) return UpscaleStatus::Ready;
    if (!device_ || !context_) return UpscaleStatus::Unsupported;
    auto adapter = adapter_of(device_.Get());
    if (!adapter) return UpscaleStatus::Unsupported;
    if (runtime_.load(directory_) != UpscaleStatus::Ready) return UpscaleStatus::UnavailableDependency;
    HANDLE handle = nullptr;
    if (!fence_ && FAILED(device_->CreateFence(0, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(&fence_))))
        return UpscaleStatus::Unsupported;
    if (FAILED(fence_->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, &handle))) return UpscaleStatus::Unsupported;
    dx12_ = Dx12Context::create(adapter.Get(), handle);
    CloseHandle(handle);
    return dx12_ ? UpscaleStatus::Ready : UpscaleStatus::UnavailableDependency;
}

std::vector<ProviderVersion> Dx12Backend::providers() {
    if (prepare() != UpscaleStatus::Ready) return {};
    return runtime_.versions(dx12_->device());
}

UpscaleStatus Dx12Backend::create_textures(const OpenParameters& parameters) noexcept {
    auto* host = device_.Get();
    auto* device = dx12_->device();
    bool created = create_texture(color_, host, device, dx12_color_format, parameters.max_render, 0) &&
        create_texture(depth_, host, device, dx12_depth_format, parameters.max_render, 0) &&
        create_texture(motion_, host, device, dx12_motion_format, parameters.max_render, 0) &&
        create_texture(reactive_, host, device, dx12_reactive_format, parameters.max_render, 0) &&
        create_texture(output_, host, device, dx12_output_format, parameters.max_output, D3D11_BIND_UNORDERED_ACCESS);
    return created ? UpscaleStatus::Ready : UpscaleStatus::Unsupported;
}

UpscaleStatus Dx12Backend::open(const ProviderVersion& provider, const OpenParameters& parameters) {
    if (auto status = prepare(); status != UpscaleStatus::Ready) return status;
    if (auto status = create_textures(parameters); status != UpscaleStatus::Ready) return status;
    if (auto status = runtime_.create(dx12_->device(), provider, parameters); status != UpscaleStatus::Ready)
        return status;
    name_ = runtime_.provider_name();
    return UpscaleStatus::Ready;
}

bool Dx12Backend::accepts(const FrameContract& contract, const FrameResources& resources) const noexcept {
    if (!color_ || !depth_ || !motion_ || !output_) return false;
    bool sizes = contract.render.width == color_->width && contract.render.height == color_->height &&
        contract.output.width == output_->width && contract.output.height == output_->height;
    bool reactive = !resources.reactive || (reactive_ && matches(*reactive_, resources.reactive));
    return sizes && reactive && matches(*color_, resources.color) && matches(*depth_, resources.depth) &&
        matches(*motion_, resources.motion) && matches(*output_, resources.output);
}

UpscaleStatus Dx12Backend::record(const FrameContract& contract, bool use_reactive) noexcept {
    auto* list = dx12_->list();
    dx12_->transition(output_->device.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    auto status = runtime_.dispatch({list, color_->device.Get(), depth_->device.Get(), motion_->device.Get(),
        output_->device.Get(), use_reactive ? reactive_->device.Get() : nullptr}, contract);
    dx12_->transition(output_->device.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
    return status;
}

UpscaleStatus Dx12Backend::upscale(const FrameContract& contract, const FrameResources& resources) {
    if (!accepts(contract, resources)) return UpscaleStatus::InvalidInput;
    context_->CopyResource(color_->host.Get(), resources.color);
    context_->CopyResource(depth_->host.Get(), resources.depth);
    context_->CopyResource(motion_->host.Get(), resources.motion);
    if (resources.reactive) context_->CopyResource(reactive_->host.Get(), resources.reactive);
    if (FAILED(context_->Signal(fence_.Get(), ++fence_value_))) return UpscaleStatus::DeviceLost;
    context_->Flush();
    auto status = record(contract, resources.reactive != nullptr);
    if (!dx12_->submit(fence_value_)) return UpscaleStatus::DeviceLost;
    if (status != UpscaleStatus::Ready) return status;
    context_->CopyResource(resources.output, output_->host.Get());
    return UpscaleStatus::Ready;
}

void Dx12Backend::close() {
    runtime_.destroy();
    color_.reset();
    depth_.reset();
    motion_.reset();
    reactive_.reset();
    output_.reset();
    name_.clear();
}
}
