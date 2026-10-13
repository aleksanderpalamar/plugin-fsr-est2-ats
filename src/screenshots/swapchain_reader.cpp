#include "screenshots/swapchain_reader.hpp"
#include "common/logger.hpp"

#include <windows.h>
#include <cstdio>

namespace neuralfx {
namespace {
bool game_window(HWND window) noexcept {
    if (!window || !IsWindowVisible(window)) return false;
    DWORD process = 0;
    GetWindowThreadProcessId(window, &process);
    if (process != GetCurrentProcessId()) return false;
    auto foreground = GetForegroundWindow();
    return foreground && GetAncestor(window, GA_ROOT) == GetAncestor(foreground, GA_ROOT);
}

void log_prepare_error(HRESULT result) noexcept {
    char message[128]{};
    std::snprintf(message, sizeof(message), "Steam screenshot backbuffer unavailable: HRESULT=0x%08lx",
        static_cast<unsigned long>(result));
    log(message);
}
}

Microsoft::WRL::ComPtr<ID3D11Texture2D> SwapchainScreenshotReader::backbuffer() {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    if (!swapchain_) return texture;
    HRESULT result = swapchain_->GetBuffer(0, IID_PPV_ARGS(&texture));
    if (FAILED(result)) log_prepare_error(result);
    return texture;
}

bool SwapchainScreenshotReader::prepare() {
    if (!swapchain_) return false;
    auto now = std::chrono::steady_clock::now();
    if (now < next_prepare_) return false;
    next_prepare_ = now + std::chrono::seconds(1);
    DXGI_SWAP_CHAIN_DESC description{};
    if (FAILED(swapchain_->GetDesc(&description)) || !game_window(description.OutputWindow)) return false;
    auto texture = backbuffer();
    if (!texture) return false;
    HRESULT result = readback_.prepare(texture.Get());
    if (FAILED(result)) {
        log_prepare_error(result);
        return false;
    }
    selected_ = swapchain_;
    window_ = description.OutputWindow;
    return true;
}

std::optional<RgbImage> SwapchainScreenshotReader::capture() {
    auto texture = backbuffer();
    if (!texture) return std::nullopt;
    return readback_.capture(texture.Get());
}
}
