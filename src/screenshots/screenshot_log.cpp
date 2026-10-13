#include "screenshots/screenshot_log.hpp"
#include "common/logger.hpp"

#include <cstdio>

namespace neuralfx {
void ScreenshotLog::report(ScreenshotEvent event, std::uint32_t handle, int result) noexcept {
    const char* description = "unknown";
    switch (event) {
    case ScreenshotEvent::Activated: description = "final-frame capture enabled; HookScreenshots(true) confirmed"; break;
    case ScreenshotEvent::Conflict: description = "game screenshot callback was not intercepted; integration skipped"; break;
    case ScreenshotEvent::ReadFailed: description = "readback failed; previous screenshot handling restored"; break;
    case ScreenshotEvent::WriteFailed: description = "WriteScreenshot returned INVALID_SCREENSHOT_HANDLE; previous handling restored"; break;
    case ScreenshotEvent::Lost: description = "capture unavailable; previous screenshot handling restored"; break;
    case ScreenshotEvent::Submitted: description = "processed RGB submitted to WriteScreenshot"; break;
    case ScreenshotEvent::Ready: description = "ScreenshotReady success"; break;
    case ScreenshotEvent::ReadyFailed: description = "ScreenshotReady failed"; break;
    case ScreenshotEvent::FallbackRequested: description = "pending screenshots reissued to previous handling via TriggerScreenshot"; break;
    case ScreenshotEvent::FallbackFailed: description = "unable to reissue pending screenshots; Steam API unavailable"; break;
    }
    char message[256]{};
    std::snprintf(message, sizeof(message), "Steam screenshot: %s handle=%lu result=%d",
        description, static_cast<unsigned long>(handle), result);
    log(message);
}
}
