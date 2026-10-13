#include "screenshots/integration.hpp"
#include "screenshots/coordinator.hpp"
#include "screenshots/screenshot_log.hpp"
#include "screenshots/steam_loader.hpp"
#include "screenshots/steam_interception.hpp"
#include "screenshots/swapchain_reader.hpp"
#include "screenshots/swapchain_selection.hpp"
#include "common/logger.hpp"

#include <mutex>

namespace neuralfx {
namespace {
class Integration {
public:
    void present(IDXGISwapChain* swapchain, FrameStatus frame) {
        std::unique_lock lock(mutex_, std::try_to_lock);
        if (!lock.owns_lock()) return;
        if (replaced(swapchain)) {
            coordinator_.reset(nullptr);
            reader_.reset();
        }
        loader_.poll();
        reader_.bind(swapchain);
        coordinator_.present(swapchain, frame, reader_);
        update_selection();
    }
    void reset(IDXGISwapChain* swapchain) {
        std::lock_guard lock(mutex_);
        if (!coordinator_.reset(swapchain)) return;
        reader_.reset();
        std::lock_guard selection_lock(selection_mutex_);
        selection_.reset();
    }
    void created(IDXGISwapChain* swapchain) {
        if (!swapchain) return;
        DXGI_SWAP_CHAIN_DESC description{};
        if (FAILED(swapchain->GetDesc(&description))) return;
        std::lock_guard lock(selection_mutex_);
        selection_.created(swapchain, description.OutputWindow);
    }

private:
    bool replaced(IDXGISwapChain* swapchain) {
        std::lock_guard lock(selection_mutex_);
        return selection_.presenting(swapchain);
    }
    void update_selection() {
        if (!reader_.selected_swapchain()) return;
        std::lock_guard lock(selection_mutex_);
        selection_.activated(reader_.selected_swapchain(), reader_.selected_window());
    }
    std::mutex mutex_;
    std::mutex selection_mutex_;
    ScreenshotSwapchainSelection selection_;
    SteamScreenshotLoader loader_;
    SteamScreenshotTransport transport_{loader_.functions(), game_steam_screenshot_relays()};
    ScreenshotLog observer_;
    ScreenshotCoordinator coordinator_{transport_, observer_};
    SwapchainScreenshotReader reader_;
};

Integration& integration() {
    static auto* instance = new Integration;
    return *instance;
}
}

void present_steam_screenshot(IDXGISwapChain* swapchain, FrameStatus frame) noexcept {
    try {
        integration().present(swapchain, frame);
    } catch (...) {
        log("Steam screenshot integration failed unexpectedly");
    }
}

void reset_steam_screenshots(IDXGISwapChain* swapchain) noexcept {
    try {
        integration().reset(swapchain);
    } catch (...) {
        log("Steam screenshot reset failed unexpectedly");
    }
}

void notify_steam_swapchain(IDXGISwapChain* swapchain) noexcept {
    try {
        integration().created(swapchain);
    } catch (...) {
        log("Steam screenshot swapchain notification failed");
    }
}
}
