#include "diagnostics/constant_dump.hpp"
#include "diagnostics/constant_dump_format.hpp"
#include "diagnostics/trace_constants.hpp"

#include <utility>

namespace neuralfx::diagnostics {
namespace {
constexpr std::uint32_t dumped_slots = 8;
constexpr std::uint32_t dumped_floats = 256;
}

void ConstantDump::arm(std::uint32_t frames) noexcept {
    std::lock_guard lock(mutex_);
    remaining_ = frames;
}

temporal::DrawDecision ConstantDump::first_draw(ID3D11DeviceContext* context, const temporal::PassEvent& event) noexcept {
    std::lock_guard lock(mutex_);
    if (remaining_ == 0) return temporal::DrawDecision::Proceed;
    try {
        for (auto stage : {ShaderStage::Vertex, ShaderStage::Pixel})
            for (const auto& snapshot : read_constants(context, stage, dumped_slots, dumped_floats))
                for (auto& line : constant_lines(event.frame, event.role, snapshot)) lines_.push_back(std::move(line));
    } catch (...) {
        lines_.clear();
    }
    return temporal::DrawDecision::Proceed;
}

std::vector<std::string> ConstantDump::finish_frame() noexcept {
    std::lock_guard lock(mutex_);
    if (remaining_ > 0) --remaining_;
    return std::exchange(lines_, {});
}
}
