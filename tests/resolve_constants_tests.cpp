#include "temporal/jitter_sequence.hpp"
#include "temporal/resolve_constants.hpp"

#include <array>
#include <cmath>
#include <iostream>

using namespace neuralfx::temporal;

namespace {
constexpr float degrees = 3.14159265f / 180.0f;
constexpr RenderSize size{1440, 1080};

using Values = std::array<float, resolve_floats>;

Values game_values() {
    Values values{};
    values[16] = 1.0f;
    values[21] = 1.0f;
    values[26] = 1.0f;
    values[44] = 1440.0f;
    values[45] = 1080.0f;
    return values;
}

void turn_by(Values& values, float yaw) {
    float c = std::cos(yaw * degrees);
    float s = std::sin(yaw * degrees);
    values[16] = c;
    values[18] = s;
    values[24] = -s;
    values[26] = c;
}

bool close_to(float actual, float expected, float tolerance = 1e-3f) {
    return std::fabs(actual - expected) < tolerance;
}

const ResolveFrame* frame_of(const std::variant<ResolveFrame, ResolveFault>& result) {
    return std::get_if<ResolveFrame>(&result);
}

int logged_frame_case() {
    Values values = game_values();
    values[18] = 9.536743e-07f;
    values[23] = 6.67572e-06f;
    values[24] = -9.536743e-07f;
    values[32] = 4.064772e-05f;
    values[33] = 0.00014992246f;
    auto result = evaluate_resolve(values, size);
    const auto* frame = frame_of(result);
    if (!frame) return 10;
    if (!close_to(frame->jitter.x, 0.05853f) || !close_to(frame->jitter.y, 0.16192f)) return 11;
    if (sequence_phase(frame->jitter) != 11u) return 12;
    if (frame->move_m > 1e-4f || frame->turn_deg > 1e-3f) return 13;
    return 0;
}

int driving_case() {
    Values values = game_values();
    turn_by(values, 2.0f);
    values[19] = 0.1f;
    values[27] = 0.3f;
    auto result = evaluate_resolve(values, size);
    const auto* frame = frame_of(result);
    if (!frame) return 20;
    if (!close_to(frame->move_m, 0.316228f)) return 21;
    return close_to(frame->turn_deg, 2.0f) ? 0 : 22;
}

int small_turn_case() {
    Values values = game_values();
    turn_by(values, 0.01f);
    auto result = evaluate_resolve(values, size);
    const auto* frame = frame_of(result);
    if (!frame) return 30;
    return close_to(frame->turn_deg, 0.01f, 1e-4f) ? 0 : 31;
}

int fault_case() {
    Values values = game_values();
    auto mismatch = evaluate_resolve(values, {1920, 1080});
    if (!std::holds_alternative<ResolveFault>(mismatch) || std::get<ResolveFault>(mismatch) != ResolveFault::SizeMismatch)
        return 40;
    auto shorter = evaluate_resolve(std::span<const float>(values).first(45), size);
    if (!std::holds_alternative<ResolveFault>(shorter) || std::get<ResolveFault>(shorter) != ResolveFault::TooShort)
        return 41;
    values[32] = std::nanf("");
    auto broken = evaluate_resolve(values, size);
    if (!std::holds_alternative<ResolveFault>(broken) || std::get<ResolveFault>(broken) != ResolveFault::NotFinite)
        return 42;
    return 0;
}
}

int main() {
    for (auto test_case : {logged_frame_case, driving_case, small_turn_case, fault_case}) {
        if (int code = test_case(); code != 0) {
            std::cerr << "resolve_constants case failed: " << code << '\n';
            return code;
        }
    }
    return 0;
}
