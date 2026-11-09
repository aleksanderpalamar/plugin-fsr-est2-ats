#include "diagnostics/pipeline_state.hpp"

#include <algorithm>
#include <cstring>

namespace neuralfx::diagnostics {
namespace {
constexpr std::size_t stored_buffer_limit = 4096;

template <size_t Count>
void assign(std::array<std::uint64_t, Count>& slots, std::uint32_t start, std::span<const std::uint64_t> ids) noexcept {
    for (size_t index = 0; index < ids.size() && start + index < Count; ++index) slots[start + index] = ids[index];
}
}

void PipelineState::set_resources(std::uint32_t start, std::span<const std::uint64_t> ids) noexcept {
    assign(resources_, start, ids);
}

void PipelineState::set_constant_buffers(std::uint32_t start, std::span<const std::uint64_t> ids) noexcept {
    assign(buffers_, start, ids);
}

void PipelineState::store_constants(std::uint64_t buffer, const void* data, std::size_t size) {
    if (!data || (contents_.size() >= stored_buffer_limit && !contents_.contains(buffer))) return;
    Stored stored{static_cast<std::uint32_t>(size), std::vector<float>(std::min(size / sizeof(float), stored_floats_))};
    std::memcpy(stored.values.data(), data, stored.values.size() * sizeof(float));
    contents_[buffer] = std::move(stored);
}

std::vector<std::uint64_t> PipelineState::inputs() const {
    std::vector<std::uint64_t> ids;
    std::copy_if(resources_.begin(), resources_.end(), std::back_inserter(ids), [](std::uint64_t id) { return id != 0; });
    return ids;
}

std::vector<ConstantSnapshot> PipelineState::constants(std::uint32_t slots) const {
    std::vector<ConstantSnapshot> snapshots;
    for (std::uint32_t slot = 0; slot < slots && slot < buffers_.size(); ++slot) {
        if (buffers_[slot] == 0) continue;
        auto found = contents_.find(buffers_[slot]);
        if (found == contents_.end()) {
            snapshots.push_back({slot, buffers_[slot], 0, {}});
            continue;
        }
        snapshots.push_back({slot, buffers_[slot], found->second.size, found->second.values});
    }
    return snapshots;
}

void PipelineState::reset() noexcept {
    resources_.fill(0);
    buffers_.fill(0);
    contents_.clear();
    pixel_shader_ = 0;
}
}
