add_executable(neuralfx_camera_motion_pass_tests
    tests/camera_motion_pass_tests.cpp
    src/renderer/camera_motion_pass.cpp
    src/renderer/fullscreen_draw.cpp
    src/gpu/shader_compiler.cpp
    src/common/logger.cpp
)
target_compile_features(neuralfx_camera_motion_pass_tests PRIVATE cxx_std_20)
target_compile_definitions(neuralfx_camera_motion_pass_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_camera_motion_pass_tests PRIVATE src)
target_link_libraries(neuralfx_camera_motion_pass_tests PRIVATE neuralfx_temporal d3d11 ${D3DCOMPILER_LIBRARY})
add_dependencies(neuralfx_camera_motion_pass_tests neuralfx_runtime_assets)
add_test(NAME neuralfx_camera_motion_pass_tests COMMAND neuralfx_camera_motion_pass_tests)
