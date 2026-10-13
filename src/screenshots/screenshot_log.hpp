#pragma once

#include "screenshots/interfaces.hpp"

namespace neuralfx {
class ScreenshotLog final : public ScreenshotObserver {
public:
    void report(ScreenshotEvent event, std::uint32_t handle, int result) noexcept override;
};
}
