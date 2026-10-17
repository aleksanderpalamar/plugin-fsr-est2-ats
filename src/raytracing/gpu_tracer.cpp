#include "raytracing/gpu_tracer.hpp"
#include "raytracing/gpu_readback.hpp"
#include "gpu/shader_compiler.hpp"

namespace neuralfx::raytracing {
namespace {
constexpr UINT thread_group_size = 8;
}

HRESULT GpuRaytracer::initialize(ID3D11Device* device, const std::filesystem::path& shader_directory) {
    reset();
    if (!device) return E_POINTER;
    HRESULT result = create_compute_shader(device, shader_directory / L"scene_tracer.hlsl",
        "TraceSceneMain", shader_);
    if (FAILED(result)) return result;
    D3D11_BUFFER_DESC description{};
    description.ByteWidth = sizeof(GpuTraceConstants);
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    result = device->CreateBuffer(&description, nullptr, &constants_);
    if (FAILED(result)) return result;
    device_ = device;
    return S_OK;
}

HRESULT GpuRaytracer::upload(const GpuSceneData& scene) {
    if (!device_) return E_FAIL;
    return scene_.upload(device_.Get(), scene);
}

std::optional<Image> GpuRaytracer::render(ID3D11DeviceContext* context, const Camera& camera,
    int width, int height, const Settings& settings) {
    if (!context || !shader_ || !scene_.ready()) return std::nullopt;
    auto constants = make_trace_constants(camera, width, height, settings, scene_.counts());
    if (!constants) return std::nullopt;
    if (FAILED(prepare_output(constants->width, constants->height))) return std::nullopt;
    context->UpdateSubresource(constants_.Get(), 0, nullptr, &*constants, 0, 0);
    dispatch(context, constants->width, constants->height);
    return read_image(device_.Get(), context, output_.Get(), width, height);
}

void GpuRaytracer::reset() noexcept {
    scene_.reset();
    output_view_.Reset();
    output_.Reset();
    constants_.Reset();
    shader_.Reset();
    device_.Reset();
    output_width_ = 0;
    output_height_ = 0;
}

HRESULT GpuRaytracer::prepare_output(UINT width, UINT height) {
    if (output_ && output_width_ == width && output_height_ == height) return S_OK;
    output_view_.Reset();
    output_.Reset();
    D3D11_TEXTURE2D_DESC description{};
    description.Width = width;
    description.Height = height;
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    HRESULT result = device_->CreateTexture2D(&description, nullptr, &output_);
    if (FAILED(result)) return result;
    result = device_->CreateUnorderedAccessView(output_.Get(), nullptr, &output_view_);
    if (FAILED(result)) {
        output_.Reset();
        return result;
    }
    output_width_ = width;
    output_height_ = height;
    return S_OK;
}

void GpuRaytracer::dispatch(ID3D11DeviceContext* context, UINT width, UINT height) noexcept {
    auto views = scene_.views();
    ID3D11UnorderedAccessView* output = output_view_.Get();
    ID3D11Buffer* constants = constants_.Get();
    context->CSSetShader(shader_.Get(), nullptr, 0);
    context->CSSetShaderResources(0, static_cast<UINT>(views.size()), views.data());
    context->CSSetUnorderedAccessViews(0, 1, &output, nullptr);
    context->CSSetConstantBuffers(0, 1, &constants);
    context->Dispatch((width + thread_group_size - 1) / thread_group_size,
        (height + thread_group_size - 1) / thread_group_size, 1);
    decltype(views) empty_views{};
    ID3D11UnorderedAccessView* empty_output = nullptr;
    ID3D11Buffer* empty_constants = nullptr;
    context->CSSetShaderResources(0, static_cast<UINT>(empty_views.size()), empty_views.data());
    context->CSSetUnorderedAccessViews(0, 1, &empty_output, nullptr);
    context->CSSetConstantBuffers(0, 1, &empty_constants);
    context->CSSetShader(nullptr, nullptr, 0);
}
}
