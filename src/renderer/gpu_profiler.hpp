#pragma once

#include <d3d11.h>
#include <wrl/client.h>

namespace neuralfx {
class GpuProfiler {
public:
    HRESULT initialize(ID3D11Device* device) noexcept;
    void begin(ID3D11DeviceContext* context) noexcept;
    void end(ID3D11DeviceContext* context) noexcept;
    void poll(ID3D11DeviceContext* context) noexcept;
    void reset() noexcept;
    double average_ms() const noexcept { return average_ms_; }

private:
    struct Slot {
        Microsoft::WRL::ComPtr<ID3D11Query> disjoint;
        Microsoft::WRL::ComPtr<ID3D11Query> start;
        Microsoft::WRL::ComPtr<ID3D11Query> finish;
        bool pending = false;
    };

    Slot slots_[3];
    unsigned current_ = 0;
    unsigned samples_ = 0;
    bool running_ = false;
    bool ready_ = false;
    double average_ms_ = 0;
};
}
