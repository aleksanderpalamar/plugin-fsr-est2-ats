#pragma once

#include <d3d11.h>
#include <array>
#include <wrl/client.h>

namespace neuralfx {
class ComputeState {
public:
    explicit ComputeState(ID3D11DeviceContext* context) noexcept;
    ~ComputeState();
    ComputeState(const ComputeState&) = delete;
    ComputeState& operator=(const ComputeState&) = delete;

    static constexpr UINT slots = 2;

private:
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader_;
    std::array<Microsoft::WRL::ComPtr<ID3D11Buffer>, slots> constants_;
    std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, slots> resources_;
    std::array<Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>, slots> outputs_;
};
}
