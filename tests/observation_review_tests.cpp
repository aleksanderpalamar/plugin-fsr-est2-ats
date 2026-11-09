#include "temporal/observation_review.hpp"

#include <cmath>
#include <vector>

using namespace neuralfx::temporal;

namespace {
constexpr RenderSize scene{1440, 1080};

constexpr Matrix4 view{-0.47095215f, 0.0f, -0.8821588f, -46.862404f,
    0.0f, 1.0f, 0.0f, 10.617724f,
    0.8821588f, 0.0f, -0.47095215f, -48.97045f,
    0.0f, 0.0f, 0.0f, 1.0f};

constexpr Matrix4 view_projection{-0.5872703f, 0.0f, -1.1014271f, -58.526123f,
    -0.0006845583f, 2.2190397f, 0.0003654605f, 23.599154f,
    0.00011026186f, 0.0f, -5.8864753e-05f, 0.19390413f,
    -0.8821588f, 0.0f, 0.47095215f, 48.97045f};

Matrix4 shifted(float x) {
    Matrix4 moved = view;
    moved[3] += x;
    return moved;
}

CameraObservation seen(std::uint64_t frame, const Matrix4& camera_view, const Matrix4& projection) {
    return {frame, {camera_view, projection}};
}

int accepted_case() {
    ObservationReview review;
    std::vector<CameraObservation> observations{seen(4, view, view_projection), seen(5, shifted(0.5f), view_projection)};
    auto report = review.review(observations, scene);
    if (report.jitters.size() != 2 || report.jitters[1].frame != 5) return 10;
    if (std::fabs(report.jitters[0].jitter.x + 0.4707f) > 2e-3f) return 11;
    if (report.steps.size() != 1 || report.jump || !report.rejections.empty() || !report.unmatched.empty()) return 12;
    if (!report.farthest_m || std::fabs(*report.farthest_m - 68.61f) > 0.1f) return 13;
    return 0;
}

int jump_case() {
    ObservationReview review;
    review.review(std::vector<CameraObservation>{seen(4, view, view_projection)}, scene);
    auto report = review.review(std::vector<CameraObservation>{seen(5, shifted(12.0f), view_projection)}, scene);
    return report.jump && report.steps.size() == 1 ? 0 : 20;
}

int rejected_case() {
    ObservationReview review;
    Matrix4 flat = view_projection;
    flat[12] = 0.0f;
    flat[14] = 0.0f;
    flat[15] = 1.0f;
    auto report = review.review(std::vector<CameraObservation>{seen(7, view, flat)}, scene);
    if (!report.jitters.empty() || report.rejections.size() != 1) return 30;
    if (report.rejections[0].frame != 7 || report.rejections[0].rejection.fault != CameraFault::NotPerspective)
        return 31;
    return report.farthest_m ? 0 : 32;
}

int unmatched_case() {
    ObservationReview review;
    Matrix4 shifted_jitter = view_projection;
    for (int column = 0; column < 4; ++column) shifted_jitter[column] -= 1.4e-4f * view[8 + column];
    auto report = review.review(std::vector<CameraObservation>{seen(9, view, shifted_jitter),
        seen(10, view, view_projection)}, scene);
    if (report.jitters.size() != 2 || report.unmatched.size() != 1) return 50;
    const auto& unmatched = report.unmatched[0];
    if (unmatched.frame != 9 || std::fabs(unmatched.jitter.x + 0.4707f) < 0.05f) return 51;
    return unmatched.matrices.view_projection[0] == shifted_jitter[0] ? 0 : 52;
}

int empty_case() {
    ObservationReview review;
    auto report = review.review({}, scene);
    return report.jitters.empty() && !report.jump && !report.farthest_m ? 0 : 40;
}
}

int main() {
    for (auto test_case : {accepted_case, jump_case, rejected_case, unmatched_case, empty_case}) {
        if (int code = test_case(); code != 0) return code;
    }
    return 0;
}
