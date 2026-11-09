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

std::string list(const std::vector<std::uint64_t>& ids, std::uint64_t backbuffer) {
    std::string text;
    for (size_t index = 0; index < ids.size(); ++index) text += (index > 0 ? "," : "") + name(ids[index], backbuffer);
    return text;
}

std::string pass(const TraceEntry& entry, std::uint64_t backbuffer) {
    std::string text = " pass";
    if (entry.kind == EntryKind::InheritedPass) text += " inherited";
    for (const auto& color : entry.colors) text += " rt=" + name(color.id, backbuffer);
    if (entry.depth) text += " ds=" + name(entry.depth->id, backbuffer);
    if (entry.viewport)
        text += " vp=" + decimal(entry.viewport->width) + "x" + decimal(entry.viewport->height);
    return text + " draws=" + std::to_string(entry.draws) +
        (entry.reads.empty() ? std::string() : " reads=" + list(entry.reads, backbuffer));
}

std::string draw(const DrawDetail& detail, std::uint64_t backbuffer) {
    std::string text = " draw count=" + std::to_string(detail.count) +
        " ps=" + (detail.shader ? hex(detail.shader) : std::string("none"));
    return detail.inputs.empty() ? text : text + " inputs=" + list(detail.inputs, backbuffer);
}

std::string constants(const ConstantSnapshot& snapshot) {
    std::string values;
    for (size_t index = 0; index < snapshot.values.size(); ++index)
        values += (index > 0 ? "," : "") + decimal(snapshot.values[index]);
    return " constants vs slot=" + std::to_string(snapshot.slot) + " buffer=" + hex(snapshot.buffer) +
        " offset=" + std::to_string(snapshot.offset) + " size=" + std::to_string(snapshot.size) + " values=" + (values.empty() ? "unknown" : values);
}

std::string body(const TraceEntry& entry, std::uint64_t backbuffer) {
    switch (entry.kind) {
    case EntryKind::Copy:
        return " copy src=" + name(entry.source, backbuffer) + " dst=" + name(entry.destination, backbuffer);
    case EntryKind::ClearColor:
        return " clear-color rt=" + name(entry.destination, backbuffer);
    case EntryKind::ClearDepth:
        return " clear-depth ds=" + name(entry.destination, backbuffer);
    case EntryKind::Draw:
        return draw(*entry.detail, backbuffer);
    case EntryKind::Constants:
        return constants(*entry.constants);
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
