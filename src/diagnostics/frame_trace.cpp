#include "diagnostics/frame_trace.hpp"

#include <algorithm>

namespace neuralfx::diagnostics {
namespace {
TraceEntry entry_of(EntryKind kind) {
    TraceEntry entry;
    entry.kind = kind;
    return entry;
}
}

void FrameTrace::begin(const TraceTarget& backbuffer) {
    backbuffer_ = backbuffer;
    entries_.clear();
    targets_.clear();
    targets_.push_back({backbuffer, TargetRole::Color, 0, 0});
    binding_ = Binding::Inherited;
    pass_ = 0;
    details_ = 0;
    dropped_ = 0;
    draws_ = 0;
}

bool FrameTrace::add(TraceEntry entry) {
    if (entries_.size() >= entry_limit_) {
        ++dropped_;
        return false;
    }
    entries_.push_back(std::move(entry));
    return true;
}

TargetUsage* FrameTrace::usage(std::uint64_t id) noexcept {
    auto found = std::find_if(targets_.begin(), targets_.end(),
        [id](const TargetUsage& usage) { return usage.target.id == id; });
    return found == targets_.end() ? nullptr : &*found;
}

void FrameTrace::record_write(const TraceTarget& target, TargetRole role) {
    if (auto* known = usage(target.id)) {
        known->target = target;
        ++known->writes;
        return;
    }
    targets_.push_back({target, role, 1, 0});
}

void FrameTrace::bind(std::span<const TraceTarget> colors, const std::optional<TraceTarget>& depth) {
    auto entry = entry_of(EntryKind::Pass);
    entry.colors.assign(colors.begin(), colors.end());
    entry.depth = depth;
    if (!add(std::move(entry))) {
        binding_ = Binding::Dropped;
        return;
    }
    binding_ = Binding::Recorded;
    pass_ = entries_.size() - 1;
    for (const auto& color : colors) record_write(color, TargetRole::Color);
    if (depth) record_write(*depth, TargetRole::Depth);
}

TraceEntry* FrameTrace::recorded_pass() noexcept {
    return binding_ == Binding::Recorded ? &entries_[pass_] : nullptr;
}

TraceEntry* FrameTrace::active_pass() {
    if (binding_ != Binding::Inherited) return recorded_pass();
    if (!add(entry_of(EntryKind::InheritedPass))) {
        binding_ = Binding::Dropped;
        return nullptr;
    }
    binding_ = Binding::Recorded;
    pass_ = entries_.size() - 1;
    return recorded_pass();
}

void FrameTrace::viewport(const TraceViewport& viewport) noexcept {
    if (auto* pass = recorded_pass()) pass->viewport = viewport;
}

void FrameTrace::draw() {
    ++draws_;
    if (auto* pass = active_pass()) ++pass->draws;
}

void FrameTrace::read(const TraceTarget& target, TargetRole role) {
    auto* pass = recorded_pass();
    if (!pass) return;
    if (!usage(target.id)) targets_.push_back({target, role, 0, 0});
    auto* known = usage(target.id);
    if (std::find(pass->reads.begin(), pass->reads.end(), target.id) != pass->reads.end()) return;
    pass->reads.push_back(target.id);
    ++known->reads;
}

void FrameTrace::copy(std::uint64_t source, const TraceTarget& destination) {
    auto* known = usage(source);
    if (!known && !usage(destination.id)) return;
    auto entry = entry_of(EntryKind::Copy);
    entry.source = source;
    entry.destination = destination.id;
    if (!add(std::move(entry))) return;
    if (known) ++known->reads;
    record_write(destination, TargetRole::Color);
}

void FrameTrace::clear_color(std::uint64_t id) {
    auto entry = entry_of(EntryKind::ClearColor);
    entry.destination = id;
    add(std::move(entry));
}

void FrameTrace::clear_depth(std::uint64_t id) {
    auto entry = entry_of(EntryKind::ClearDepth);
    entry.destination = id;
    add(std::move(entry));
}

const TraceEntry* FrameTrace::current_pass() const noexcept {
    return binding_ == Binding::Recorded ? &entries_[pass_] : nullptr;
}

bool FrameTrace::wants_draw_detail() const noexcept {
    const auto* pass = current_pass();
    if (!pass || details_ >= detail_limit_) return false;
    return std::any_of(pass->colors.begin(), pass->colors.end(),
        [this](const TraceTarget& color) { return color.id == backbuffer_.id; });
}

bool FrameTrace::wants_constants() const noexcept {
    const auto* pass = current_pass();
    if (!pass || !pass->depth || pass->draws > 0) return false;
    return pass->depth->width == backbuffer_.width && pass->depth->height == backbuffer_.height;
}

void FrameTrace::detail(DrawDetail detail) {
    auto entry = entry_of(EntryKind::Draw);
    entry.detail = std::move(detail);
    if (add(std::move(entry))) ++details_;
}

void FrameTrace::constants(ConstantSnapshot snapshot) {
    auto entry = entry_of(EntryKind::Constants);
    entry.constants = std::move(snapshot);
    add(std::move(entry));
}
}
