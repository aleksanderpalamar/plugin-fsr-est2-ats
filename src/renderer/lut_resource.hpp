#pragma once

#include <d3d11.h>
#include <filesystem>
#include <wrl/client.h>

namespace neuralfx {
class LutResource {
public:
    HRESULT load(ID3D11Device* device, const std::filesystem::path& path);
    void reset() noexcept;
    ID3D11ShaderResourceView* view() const noexcept { return view_.Get(); }
    unsigned size() const noexcept { return size_; }

private:
    Microsoft::WRL::ComPtr<ID3D11Texture3D> texture_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view_;
    unsigned size_ = 0;
};
}
