#pragma once

#include "screenshots/steam_callback.hpp"

#include <cstdint>

extern "C" {
__declspec(dllimport) void SteamAPI_RegisterCallback(neuralfx::SteamCallback*, int);
__declspec(dllimport) void SteamAPI_UnregisterCallback(neuralfx::SteamCallback*);
__declspec(dllimport) void SteamAPI_RunCallbacks();
__declspec(dllimport) void* SteamAPI_SteamScreenshots_v003();
__declspec(dllimport) void SteamAPI_ISteamScreenshots_HookScreenshots(void*, bool);
__declspec(dllimport) bool SteamAPI_ISteamScreenshots_IsScreenshotsHooked(void*);
__declspec(dllimport) std::uint32_t SteamAPI_ISteamScreenshots_WriteScreenshot(
    void*, void*, std::uint32_t, int, int);
__declspec(dllimport) void ScreenshotTest_Request();
__declspec(dllimport) int ScreenshotTest_Writes();
__declspec(dllimport) int ScreenshotTest_Channel(int);
__declspec(dllimport) int ScreenshotTest_Width();
__declspec(dllimport) int ScreenshotTest_Height();
}
