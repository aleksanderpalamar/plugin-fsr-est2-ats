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
    void first_draw(ID3D11DeviceContext* context, std::uint64_t frame, temporal::PassRole role) noexcept override;
    std::vector<std::string> finish_frame() noexcept;

private:
    std::mutex mutex_;
    std::uint32_t remaining_ = 0;
    std::vector<std::string> lines_;
};
}
