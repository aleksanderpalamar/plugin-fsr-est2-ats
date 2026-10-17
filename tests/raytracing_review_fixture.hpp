#pragma once

#include "common/logger.hpp"
#include "gpu/shader_compiler.hpp"
#include "renderer/fullscreen_draw.hpp"
#include "screen_space_pass_fixture.hpp"

namespace neuralfx::testing {
inline ComPtr<ID3D11PixelShader> review_shader(Fixture& fixture, const wchar_t* filename,
    const char* entry) {
    ComPtr<ID3D11PixelShader> shader;
    auto path = module_directory() / L"NeuralFX" / L"shaders" / filename;
    require(SUCCEEDED(create_pixel_shader(fixture.gpu.device.Get(), path, entry, shader)),
        std::string("pixel shader created: ") + entry);
    return shader;
}

inline void draw_review(Fixture& fixture, ID3D11PixelShader* shader,
    ID3D11ShaderResourceView* color, ID3D11ShaderResourceView* depth,
    ID3D11RenderTargetView* output, const ScreenSpaceConstants& constants) {
    auto* device = fixture.gpu.device.Get();
    ComPtr<ID3D11VertexShader> vertex;
    auto path = module_directory() / L"NeuralFX" / L"shaders" / L"fullscreen.hlsl";
    require(SUCCEEDED(create_vertex_shader(device, path, "VSMain", vertex)), "vertex shader created");
    D3D11_BUFFER_DESC description{};
    description.ByteWidth = sizeof(constants);
    description.Usage = D3D11_USAGE_IMMUTABLE;
    description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA initial{&constants, 0, 0};
    ComPtr<ID3D11Buffer> buffer;
    require(SUCCEEDED(device->CreateBuffer(&description, &initial, &buffer)), "constants uploaded");
    draw_fullscreen(fixture.gpu.context.Get(), {vertex.Get(), shader, {color, depth, nullptr},
        output, buffer.Get(), nullptr, width, height});
}

inline FloatTarget review_output(Fixture& fixture, const SyntheticScene& scene,
    const ScreenSpaceConstants& constants, ID3D11PixelShader* shader = nullptr) {
    auto* device = fixture.gpu.device.Get();
    auto color = float_color(fixture, scene);
    auto depth = create_input(device, width, height, DXGI_FORMAT_R32_FLOAT,
        scene.device_depth.data(), static_cast<UINT>(width * sizeof(float)));
    auto description = texture_description(width, height, DXGI_FORMAT_R32G32B32A32_FLOAT,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    FloatTarget target;
    require(SUCCEEDED(device->CreateTexture2D(&description, nullptr, &target.texture)),
        "review texture created");
    require(SUCCEEDED(device->CreateRenderTargetView(target.texture.Get(), nullptr, &target.view)),
        "review target created");
    require(color && depth, "review inputs created");
    if (shader) {
        draw_review(fixture, shader, color.Get(), depth.Get(), target.view.Get(), constants);
        return target;
    }
    fixture.pass.render(fixture.gpu.context.Get(),
        {color.Get(), depth.Get(), target.view.Get(), width, height}, constants);
    return target;
}

using EncodedPixel = std::array<unsigned char, 4>;

inline std::vector<EncodedPixel> read_encoded(Fixture& fixture, ID3D11Texture2D* texture) {
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    description.BindFlags = 0;
    description.Usage = D3D11_USAGE_STAGING;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;
    require(SUCCEEDED(fixture.gpu.device->CreateTexture2D(&description, nullptr, &staging)),
        "encoded staging texture created");
    auto* context = fixture.gpu.context.Get();
    context->CopyResource(staging.Get(), texture);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    require(SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)), "encoded pixels mapped");
    std::vector<EncodedPixel> pixels(static_cast<size_t>(width) * height);
    for (UINT row = 0; row < height; ++row) {
        auto* source = static_cast<const unsigned char*>(mapped.pData) + row * mapped.RowPitch;
        std::memcpy(pixels.data() + static_cast<size_t>(row) * width, source, width * sizeof(EncodedPixel));
    }
    context->Unmap(staging.Get(), 0);
    return pixels;
}

inline std::vector<EncodedPixel> display_review(Fixture& fixture, const FloatTarget& input,
    DXGI_FORMAT format, const char* entry) {
    auto* device = fixture.gpu.device.Get();
    ComPtr<ID3D11ShaderResourceView> view;
    require(SUCCEEDED(device->CreateShaderResourceView(input.texture.Get(), nullptr, &view)),
        "debug display input created");
    auto description = texture_description(width, height, format, D3D11_BIND_RENDER_TARGET);
    FloatTarget output;
    require(SUCCEEDED(device->CreateTexture2D(&description, nullptr, &output.texture)),
        "debug display texture created");
    require(SUCCEEDED(device->CreateRenderTargetView(output.texture.Get(), nullptr, &output.view)),
        "debug display target created");
    auto shader = review_shader(fixture, L"fullscreen.hlsl", entry);
    draw_review(fixture, shader.Get(), view.Get(), nullptr, output.view.Get(),
        constants_for(settings_for(Projection{})));
    return read_encoded(fixture, output.texture.Get());
}
}
