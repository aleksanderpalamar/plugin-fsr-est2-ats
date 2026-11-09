#include "diagnostics/frame_trace.hpp"

#include <algorithm>
#include <array>
#include <charconv>

namespace neuralfx::diagnostics {
namespace {
std::string hex(std::uint64_t value) {
    std::array<char, 24> buffer{};
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, 16);
    return std::string(buffer.data(), result.ptr);
}

std::string decimal(float value) {
    std::array<char, 32> buffer{};
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    return std::string(buffer.data(), result.ptr);
}

std::string name(std::uint64_t id, std::uint64_t backbuffer) {
    return id == backbuffer ? "bb" : hex(id);
}

std::string describe(const TraceTarget& target) {
    return std::to_string(target.width) + "x" + std::to_string(target.height) +
        " fmt=" + std::to_string(target.format) + " samples=" + std::to_string(target.samples);
}

std::string reads(const TraceEntry& entry, std::uint64_t backbuffer) {
    if (entry.reads.empty()) return {};
    std::string text = " reads=";
    for (size_t index = 0; index < entry.reads.size(); ++index) {
        if (index > 0) text += ",";
        text += name(entry.reads[index], backbuffer);
    }
    return text;
}

std::string pass(const TraceEntry& entry, std::uint64_t backbuffer) {
    std::string text = " pass";
    if (entry.kind == EntryKind::InheritedPass) text += " inherited";
    for (const auto& color : entry.colors) text += " rt=" + name(color.id, backbuffer);
    if (entry.depth) text += " ds=" + name(entry.depth->id, backbuffer);
    if (entry.viewport)
        text += " vp=" + decimal(entry.viewport->width) + "x" + decimal(entry.viewport->height);
    return text + " draws=" + std::to_string(entry.draws) + reads(entry, backbuffer);
}

std::string body(const TraceEntry& entry, std::uint64_t backbuffer) {
    switch (entry.kind) {
    case EntryKind::Copy:
        return " copy src=" + name(entry.source, backbuffer) + " dst=" + name(entry.destination, backbuffer);
    case EntryKind::ClearColor:
        return " clear-color rt=" + name(entry.destination, backbuffer);
    case EntryKind::ClearDepth:
        return " clear-depth ds=" + name(entry.destination, backbuffer);
    case EntryKind::Pass:
    case EntryKind::InheritedPass:
        break;
    }
    return pass(entry, backbuffer);
}

std::string role(TargetRole value) {
    return value == TargetRole::Depth ? "depth" : "color";
}
}

std::vector<std::string> FrameTrace::target_lines() const {
    std::vector<TargetUsage> ordered = targets_;
    std::stable_sort(ordered.begin() + 1, ordered.end(),
        [](const TargetUsage& left, const TargetUsage& right) { return left.target.id < right.target.id; });
    std::vector<std::string> lines;
    for (const auto& usage : ordered)
        lines.push_back("Trace target " + name(usage.target.id, backbuffer_.id) + " " +
            describe(usage.target) + " " + role(usage.role) + " writes=" + std::to_string(usage.writes) +
            " reads=" + std::to_string(usage.reads));
    return lines;
}

std::vector<std::string> FrameTrace::lines() const {
    std::vector<std::string> lines{"Trace frame begin backbuffer=bb " + describe(backbuffer_)};
    for (size_t index = 0; index < entries_.size(); ++index)
        lines.push_back("Trace " + std::to_string(index) + body(entries_[index], backbuffer_.id));
    auto targets = target_lines();
    lines.insert(lines.end(), targets.begin(), targets.end());
    lines.push_back("Trace frame end entries=" + std::to_string(entries_.size()) +
        " draws=" + std::to_string(draws_) + " dropped=" + std::to_string(dropped_));
    return lines;
}
}
