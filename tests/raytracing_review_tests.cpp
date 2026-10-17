#include "reflection_refinement_tests.hpp"
#include "screen_space_debug_tests.hpp"

#include <iostream>

using namespace neuralfx;
using namespace neuralfx::testing;

int main() {
    auto gpu = create_test_device();
    if (!gpu) {
        std::cerr << "D3D11 device unavailable\n";
        return 1;
    }
    Fixture fixture{*gpu, {}};
    auto shaders = module_directory() / L"NeuralFX" / L"shaders";
    if (FAILED(fixture.pass.initialize(gpu->device.Get(), shaders))) {
        std::cerr << "Screen-space pass initialization failed\n";
        return 2;
    }
    constexpr std::array cases{refinement_toward_cases, refinement_away_cases,
        debug_srgb_cases, debug_unorm_cases};
    int failures = 0;
    for (auto test_case : cases) {
        try {
            test_case(fixture);
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            ++failures;
        }
    }
    return failures == 0 ? 0 : 3;
}
