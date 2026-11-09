#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace neuralfx::diagnostics {
enum class TargetRole { Color, Depth };

struct TraceTarget {
    std::uint64_t id = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t format = 0;
    std::uint32_t samples = 1;
};

struct TraceViewport {
    float width = 0.0f;
    float height = 0.0f;
};

enum class EntryKind { Pass, InheritedPass, Copy, ClearColor, ClearDepth, Draw, Constants };

struct DrawDetail {
    std::uint32_t count = 0;
    std::uint64_t shader = 0;
    std::vector<std::uint64_t> inputs;
};

enum class ShaderStage { Vertex, Pixel };

struct ConstantSnapshot {
    std::uint32_t slot = 0;
    std::uint64_t buffer = 0;
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
    std::vector<float> values;
    ShaderStage stage = ShaderStage::Vertex;
};
enum class Binding { Inherited, Recorded, Dropped };

struct TraceEntry {
    EntryKind kind = EntryKind::Pass;
    std::vector<TraceTarget> colors;
    std::optional<TraceTarget> depth;
    std::optional<TraceViewport> viewport;
    std::vector<std::uint64_t> reads;
    std::uint64_t source = 0;
    std::uint64_t destination = 0;
    std::uint32_t draws = 0;
    std::optional<DrawDetail> detail;
    std::optional<ConstantSnapshot> constants;
};

struct TargetUsage {
    TraceTarget target;
    TargetRole role = TargetRole::Color;
    std::uint32_t writes = 0;
    std::uint32_t reads = 0;
};

class FrameTrace {
public:
    explicit FrameTrace(std::size_t entry_limit, std::size_t detail_limit = 256) noexcept
        : entry_limit_(entry_limit), detail_limit_(detail_limit) {}

    void begin(const TraceTarget& backbuffer);
    void bind(std::span<const TraceTarget> colors, const std::optional<TraceTarget>& depth);
    void viewport(const TraceViewport& viewport) noexcept;
    void draw();
    void read(const TraceTarget& target, TargetRole role);
    void copy(std::uint64_t source, const TraceTarget& destination);
    void clear_color(std::uint64_t id);
    void clear_depth(std::uint64_t id);
    bool wants_draw_detail() const noexcept;
    bool wants_constants() const noexcept;
    void detail(DrawDetail detail);
    void constants(ConstantSnapshot snapshot);
    std::vector<std::string> lines() const;

private:
    std::vector<std::string> target_lines() const;
    bool add(TraceEntry entry);
    TraceEntry* active_pass();
    TraceEntry* recorded_pass() noexcept;
    TargetUsage* usage(std::uint64_t id) noexcept;
    void record_write(const TraceTarget& target, TargetRole role);

    const TraceEntry* current_pass() const noexcept;

    std::size_t entry_limit_;
    std::size_t detail_limit_;
    std::size_t details_ = 0;
    TraceTarget backbuffer_;
    std::vector<TraceEntry> entries_;
    std::vector<TargetUsage> targets_;
    Binding binding_ = Binding::Inherited;
    std::size_t pass_ = 0;
    std::uint32_t dropped_ = 0;
    std::uint32_t draws_ = 0;
};
}
