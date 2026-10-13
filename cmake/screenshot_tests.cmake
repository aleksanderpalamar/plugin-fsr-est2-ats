add_library(neuralfx_screenshots STATIC
    src/screenshots/rgb_image.cpp src/screenshots/coordinator.cpp
    src/screenshots/steam_transport.cpp src/screenshots/swapchain_selection.cpp
    src/screenshots/steam_callback_relay.cpp src/screenshots/steam_callback_relays.cpp)
target_compile_features(neuralfx_screenshots PUBLIC cxx_std_20)
target_include_directories(neuralfx_screenshots PUBLIC src)

foreach(test IN ITEMS screenshot_rgb screenshot_coordinator steam_screenshot_transport
    steam_callback_abi steam_callback_relay screenshot_swapchain_selection)
    add_executable(neuralfx_${test}_tests tests/${test}_tests.cpp)
    target_link_libraries(neuralfx_${test}_tests PRIVATE neuralfx_screenshots)
    add_test(NAME neuralfx_${test}_tests COMMAND neuralfx_${test}_tests)
endforeach()

if(NOT WIN32)
    return()
endif()

add_executable(neuralfx_screenshot_readback_tests tests/screenshot_readback_tests.cpp
    src/screenshots/d3d11_readback.cpp src/common/logger.cpp)
target_link_libraries(neuralfx_screenshot_readback_tests PRIVATE neuralfx_screenshots d3d11)
target_compile_definitions(neuralfx_screenshot_readback_tests PRIVATE
    UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
set_target_properties(neuralfx_screenshot_readback_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests)
add_test(NAME neuralfx_screenshot_readback_tests COMMAND neuralfx_screenshot_readback_tests)

add_executable(neuralfx_steam_imports_tests tests/steam_imports_tests.cpp
    src/screenshots/steam_imports.cpp)
target_compile_features(neuralfx_steam_imports_tests PRIVATE cxx_std_20)
target_compile_definitions(neuralfx_steam_imports_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_steam_imports_tests PRIVATE src)
add_test(NAME neuralfx_steam_imports_tests COMMAND neuralfx_steam_imports_tests)
