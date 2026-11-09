#pragma once

#include "diagnostics/frame_trace.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace neuralfx::diagnostics {
class PipelineState {
public:
    explicit PipelineState(std::size_t stored_floats = 64) noexcept : stored_floats_(stored_floats) {}

    void set_resources(std::uint32_t start, std::span<const std::uint64_t> ids) noexcept;
    void set_pixel_shader(std::uint64_t id) noexcept { pixel_shader_ = id; }
    void set_constant_buffers(std::uint32_t start, std::span<const std::uint64_t> ids) noexcept;
    void store_constants(std::uint64_t buffer, const void* data, std::size_t size);
    std::vector<std::uint64_t> inputs() const;
    std::uint64_t pixel_shader() const noexcept { return pixel_shader_; }
    std::vector<ConstantSnapshot> constants(std::uint32_t slots) const;
    void reset() noexcept;

private:
    struct Stored {
        std::uint32_t size = 0;
        std::vector<float> values;
    };

    std::size_t stored_floats_;
    std::array<std::uint64_t, 128> resources_{};
    std::array<std::uint64_t, 14> buffers_{};
    std::unordered_map<std::uint64_t, Stored> contents_;
    std::uint64_t pixel_shader_ = 0;
};
}
