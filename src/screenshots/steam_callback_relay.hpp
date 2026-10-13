#pragma once

#include "screenshots/steam_callback.hpp"

#include <array>
#include <mutex>

namespace neuralfx {
struct SteamCallbackRegistration {
    void (*add)(SteamCallback*, int);
    void (*remove)(SteamCallback*);
};

class SteamScreenshotRelaySource {
public:
    virtual ~SteamScreenshotRelaySource() = default;
    virtual bool activate(ScreenshotEvents& events) noexcept = 0;
    virtual bool watch() noexcept = 0;
    virtual bool has_callbacks() const noexcept = 0;
    virtual bool active() const noexcept = 0;
    virtual void deactivate() noexcept = 0;
};

struct SteamCallbackRouting {
    enum class Strategy { Forward, Relay, Suppress };
    std::atomic<Strategy> strategy{Strategy::Forward};
    std::atomic<ScreenshotEvents*> events{nullptr};
    std::atomic<SteamCallback*> leader{nullptr};
};

class SteamCallbackRelay final : public SteamCallback {
public:
    SteamCallbackRelay();
    bool attach(SteamCallback* original, SteamCallbackRegistration api, SteamCallbackRouting& routing);
    bool release(SteamCallback* original, SteamCallbackRegistration api);
    bool matches(SteamCallback* original) const noexcept;
    bool occupied() const noexcept;
    bool live() const;
    bool activate(ScreenshotEvents& events);

private:
    void dispatch(void* data, bool extended, bool failed, std::uint64_t call);
    void forward(SteamCallback* original, void* data, bool extended, bool failed, std::uint64_t call);
    static void plain(SteamCallback* base, void* data);
    static void extended(SteamCallback* base, void* data, bool failed, std::uint64_t call);
    static int buffer_size(SteamCallback* base);
    inline static constexpr SteamCallbackFunctions functions_{extended, plain, buffer_size};
    mutable std::recursive_mutex mutex_;
    std::atomic<SteamCallback*> original_{nullptr};
    SteamCallbackRouting* routing_ = nullptr;
};

class SteamCallbackRelays final : public SteamScreenshotRelaySource {
public:
    explicit SteamCallbackRelays(SteamCallbackRegistration api) : api_(api) {}
    void register_callback(SteamCallback* callback, int id);
    void unregister_callback(SteamCallback* callback);
    bool activate(ScreenshotEvents& events) noexcept override;
    bool watch() noexcept override;
    bool has_callbacks() const noexcept override;
    bool active() const noexcept override;
    void deactivate() noexcept override;

private:
    SteamCallbackRelay* find(SteamCallback* callback) noexcept;
    SteamCallbackRegistration api_;
    SteamCallbackRouting routing_;
    std::array<SteamCallbackRelay, 32> relays_;
    std::atomic_bool complete_{true};
};
}
