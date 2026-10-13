add_library(neuralfx_test_steam_api SHARED tests/steam_screenshot_host_api.cpp)
target_compile_features(neuralfx_test_steam_api PRIVATE cxx_std_20)
target_include_directories(neuralfx_test_steam_api PRIVATE src)
set_target_properties(neuralfx_test_steam_api PROPERTIES OUTPUT_NAME steam_api64 PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/screenshot-host)

add_executable(neuralfx_steam_screenshot_host tests/steam_screenshot_host.cpp)
target_compile_features(neuralfx_steam_screenshot_host PRIVATE cxx_std_20)
target_include_directories(neuralfx_steam_screenshot_host PRIVATE src)
target_compile_definitions(neuralfx_steam_screenshot_host PRIVATE
    UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(neuralfx_steam_screenshot_host PRIVATE neuralfx_test_steam_api
    dxgi d3d11 dxguid user32)
set_target_properties(neuralfx_steam_screenshot_host PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/screenshot-host)
add_custom_target(neuralfx_screenshot_host_assets ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:neuralfx_steam_screenshot_host>
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:dxgi>
        $<TARGET_FILE_DIR:neuralfx_steam_screenshot_host>/$<TARGET_FILE_NAME:dxgi>
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:neuralfx_steam_screenshot_host>/NeuralFX/shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:neuralfx_steam_screenshot_host>/NeuralFX/luts
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/tests/assets/screenshot_host.ini
        $<TARGET_FILE_DIR:neuralfx_steam_screenshot_host>/NeuralFX/neuralfx.ini
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/tests/assets/screenshot.cube
        $<TARGET_FILE_DIR:neuralfx_steam_screenshot_host>/NeuralFX/luts/screenshot.cube
    DEPENDS dxgi)
add_dependencies(neuralfx_steam_screenshot_host neuralfx_screenshot_host_assets)
add_test(NAME neuralfx_steam_screenshot_host COMMAND neuralfx_steam_screenshot_host)
add_test(NAME neuralfx_steam_screenshot_host_late COMMAND neuralfx_steam_screenshot_host --late-handler)
set_tests_properties(neuralfx_steam_screenshot_host neuralfx_steam_screenshot_host_late
    PROPERTIES TIMEOUT 45 RESOURCE_LOCK screenshot_host)
