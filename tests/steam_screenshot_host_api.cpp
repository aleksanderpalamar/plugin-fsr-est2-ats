#include "screenshots/steam_callback.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace {
using neuralfx::SteamCallback;
std::vector<SteamCallback*> callbacks;
bool hooked = false;
bool pending = false;
int writes = 0;
int width = 0;
int height = 0;
std::array<int, 3> channels{};
void dispatch(int id, void* data) {
    auto registered = callbacks;
    for (auto* callback : registered) {
        if (callback->callback != id || !callback->registered()) continue;
        callback->run(data);
    }
}
}

extern "C" {
__declspec(dllexport) int SteamAPI_GetHSteamUser() { return 1; }
__declspec(dllexport) int SteamAPI_GetHSteamPipe() { return 1; }
__declspec(dllexport) void* SteamAPI_SteamScreenshots_v003() { return &hooked; }
__declspec(dllexport) void SteamAPI_RegisterCallback(SteamCallback* callback, int id) {
    callback->callback = id;
    callback->flags |= 1;
    if (std::find(callbacks.begin(), callbacks.end(), callback) != callbacks.end()) return;
    callbacks.push_back(callback);
}
__declspec(dllexport) void SteamAPI_UnregisterCallback(SteamCallback* callback) {
    callback->flags &= ~1;
    std::erase(callbacks, callback);
}
__declspec(dllexport) void SteamAPI_ISteamScreenshots_HookScreenshots(void*, bool enable) {
    hooked = enable;
}
__declspec(dllexport) bool SteamAPI_ISteamScreenshots_IsScreenshotsHooked(void*) { return hooked; }
__declspec(dllexport) std::uint32_t SteamAPI_ISteamScreenshots_WriteScreenshot(
    void*, void* rgb, std::uint32_t size, int image_width, int image_height) {
    if (!rgb || size < 3 || size != image_width * image_height * 3u) return 0;
    auto* bytes = static_cast<std::uint8_t*>(rgb);
    for (std::size_t index = 0; index < channels.size(); ++index) channels[index] = bytes[index];
    width = image_width;
    height = image_height;
    return static_cast<std::uint32_t>(++writes);
}
__declspec(dllexport) void SteamAPI_RunCallbacks() {
    if (!pending || !hooked) return;
    pending = false;
    dispatch(2302, nullptr);
}
__declspec(dllexport) void ScreenshotTest_Request() { pending = true; }
__declspec(dllexport) int ScreenshotTest_Writes() { return writes; }
__declspec(dllexport) int ScreenshotTest_Channel(int index) { return channels.at(index); }
__declspec(dllexport) int ScreenshotTest_Width() { return width; }
__declspec(dllexport) int ScreenshotTest_Height() { return height; }
}
