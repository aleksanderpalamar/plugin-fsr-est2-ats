#pragma once

#include "screenshots/rgb_image.hpp"

namespace neuralfx {
enum class ScreenshotActivation { Active, Unavailable, Conflict };
enum class ScreenshotEvent { Activated, Conflict, ReadFailed, WriteFailed, Lost,
    Submitted, Ready, ReadyFailed, FallbackRequested, FallbackFailed };

class ScreenshotObserver {
public:
    virtual ~ScreenshotObserver() = default;
    virtual void report(ScreenshotEvent event, std::uint32_t handle = 0,
        int result = 0) noexcept = 0;
};

class ScreenshotEvents {
public:
    virtual ~ScreenshotEvents() = default;
    virtual void requested() noexcept = 0;
    virtual void ready(std::uint32_t handle, int result) noexcept = 0;
};

class ScreenshotTransport {
public:
    virtual ~ScreenshotTransport() = default;
    virtual bool available() noexcept = 0;
    virtual ScreenshotActivation activate(ScreenshotEvents& events) noexcept = 0;
    virtual bool active() noexcept = 0;
    virtual void deactivate() noexcept = 0;
    virtual std::uint32_t write(const RgbImage& image) noexcept = 0;
    virtual bool retry_original(std::uint32_t requests) noexcept = 0;
};

class ScreenshotReader {
public:
    virtual ~ScreenshotReader() = default;
    virtual bool prepare() = 0;
    virtual std::optional<RgbImage> capture() = 0;
};
}
