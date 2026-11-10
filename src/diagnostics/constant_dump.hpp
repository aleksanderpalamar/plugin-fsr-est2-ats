#pragma once

#include "temporal/pass_listener.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace neuralfx::diagnostics {
class ConstantDump final : public temporal::PassListener {
public:
    void arm(std::uint32_t frames) noexcept;
    temporal::DrawDecision first_draw(ID3D11DeviceContext* context, const temporal::PassEvent& event) noexcept override;
    std::vector<std::string> finish_frame() noexcept;

private:
    std::mutex mutex_;
    std::uint32_t remaining_ = 0;
    std::vector<std::string> lines_;
};
}
