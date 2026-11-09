#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace neuralfx::diagnostics {
class PipelineState {
public:
    void set_resources(std::uint32_t start, std::span<const std::uint64_t> ids) noexcept;
    void set_pixel_shader(std::uint64_t id) noexcept { pixel_shader_ = id; }
    std::vector<std::uint64_t> inputs() const;
    std::uint64_t pixel_shader() const noexcept { return pixel_shader_; }
    void reset() noexcept;

private:
    std::array<std::uint64_t, 128> resources_{};
    std::uint64_t pixel_shader_ = 0;
};
}
