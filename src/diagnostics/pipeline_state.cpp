#include "diagnostics/pipeline_state.hpp"

#include <algorithm>
#include <iterator>

namespace neuralfx::diagnostics {
void PipelineState::set_resources(std::uint32_t start, std::span<const std::uint64_t> ids) noexcept {
    for (size_t index = 0; index < ids.size() && start + index < resources_.size(); ++index)
        resources_[start + index] = ids[index];
}

std::vector<std::uint64_t> PipelineState::inputs() const {
    std::vector<std::uint64_t> ids;
    std::copy_if(resources_.begin(), resources_.end(), std::back_inserter(ids), [](std::uint64_t id) { return id != 0; });
    return ids;
}

void PipelineState::reset() noexcept {
    resources_.fill(0);
    pixel_shader_ = 0;
}
}
