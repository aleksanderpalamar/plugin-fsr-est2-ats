#include "screenshots/swapchain_selection.hpp"

int main() {
    neuralfx::ScreenshotSwapchainSelection selection;
    int primary = 0;
    int replacement = 0;
    int secondary = 0;
    int game_window = 0;
    int other_window = 0;
    selection.created(&primary, &game_window);
    if (selection.presenting(&primary)) return 1;
    selection.activated(&primary, &game_window);
    selection.created(&secondary, &other_window);
    selection.created(&replacement, &game_window);
    if (selection.presenting(&primary)) return 2;
    if (selection.presenting(&secondary)) return 3;
    if (!selection.presenting(&replacement)) return 4;
    if (selection.presenting(&replacement)) return 5;
    if (selection.presenting(&primary)) return 6;
    selection.reset();
    if (selection.presenting(&replacement)) return 7;
    selection.created(nullptr, &game_window);
    selection.created(&replacement, nullptr);
    if (selection.presenting(&replacement)) return 8;
    return 0;
}
