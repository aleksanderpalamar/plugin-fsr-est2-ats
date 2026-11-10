add_library(neuralfx_temporal_d3d11 STATIC src/temporal/scene_tracker.cpp src/temporal/camera_capture.cpp
    src/temporal/scene_hooks.cpp src/temporal/bound_constants.cpp src/temporal/staging_ring.cpp
    src/temporal/resolve_capture.cpp)
target_compile_features(neuralfx_temporal_d3d11 PUBLIC cxx_std_20)
target_compile_definitions(neuralfx_temporal_d3d11 PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_temporal_d3d11 PUBLIC src)
target_link_libraries(neuralfx_temporal_d3d11 PUBLIC neuralfx_temporal d3d11)
target_link_libraries(dxgi PRIVATE neuralfx_temporal_d3d11)
target_link_libraries(neuralfx_depth_hook_tests PRIVATE neuralfx_temporal_d3d11)
target_link_libraries(neuralfx_frame_trace_hook_tests PRIVATE neuralfx_temporal_d3d11)

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

add_executable(neuralfx_camera_reprojection_pass_tests
    tests/camera_reprojection_pass_tests.cpp
    src/renderer/camera_reprojection_pass.cpp
    src/renderer/camera_frame_slot.cpp
    src/renderer/compute_state.cpp
    src/gpu/shader_compiler.cpp
    src/common/logger.cpp
)
target_compile_features(neuralfx_camera_reprojection_pass_tests PRIVATE cxx_std_20)
target_compile_definitions(neuralfx_camera_reprojection_pass_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_camera_reprojection_pass_tests PRIVATE src)
target_link_libraries(neuralfx_camera_reprojection_pass_tests PRIVATE neuralfx_temporal d3d11 ${D3DCOMPILER_LIBRARY})
add_dependencies(neuralfx_camera_reprojection_pass_tests neuralfx_runtime_assets)
add_test(NAME neuralfx_camera_reprojection_pass_tests COMMAND neuralfx_camera_reprojection_pass_tests)

add_executable(neuralfx_resolve_frame_pass_tests
    tests/resolve_frame_pass_tests.cpp
    src/renderer/resolve_frame_pass.cpp
    src/renderer/camera_frame_slot.cpp
    src/renderer/compute_state.cpp
    src/gpu/shader_compiler.cpp
    src/common/logger.cpp
)
target_compile_features(neuralfx_resolve_frame_pass_tests PRIVATE cxx_std_20)
target_compile_definitions(neuralfx_resolve_frame_pass_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_resolve_frame_pass_tests PRIVATE src)
target_link_libraries(neuralfx_resolve_frame_pass_tests PRIVATE neuralfx_temporal d3d11 ${D3DCOMPILER_LIBRARY})
add_dependencies(neuralfx_resolve_frame_pass_tests neuralfx_runtime_assets)
add_test(NAME neuralfx_resolve_frame_pass_tests COMMAND neuralfx_resolve_frame_pass_tests)

add_executable(neuralfx_scene_tracker_tests
    tests/scene_tracker_tests.cpp
    src/dxgi/context_hook.cpp
    src/dxgi/depth_capture.cpp
    src/renderer/depth_format.cpp
    src/common/logger.cpp
)
target_compile_definitions(neuralfx_scene_tracker_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(neuralfx_scene_tracker_tests PRIVATE neuralfx_temporal_d3d11 neuralfx_trace_d3d11
    ${D3DCOMPILER_LIBRARY} user32)
set_target_properties(neuralfx_scene_tracker_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests)
add_test(NAME neuralfx_scene_tracker_tests COMMAND neuralfx_scene_tracker_tests)

add_executable(neuralfx_motion_overlay_pass_tests
    tests/motion_overlay_pass_tests.cpp
    src/renderer/motion_overlay_pass.cpp
    src/renderer/fullscreen_draw.cpp
    src/gpu/shader_compiler.cpp
    src/common/logger.cpp
)
target_compile_features(neuralfx_motion_overlay_pass_tests PRIVATE cxx_std_20)
target_compile_definitions(neuralfx_motion_overlay_pass_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_motion_overlay_pass_tests PRIVATE src)
target_link_libraries(neuralfx_motion_overlay_pass_tests PRIVATE d3d11 ${D3DCOMPILER_LIBRARY})
add_dependencies(neuralfx_motion_overlay_pass_tests neuralfx_runtime_assets)
add_test(NAME neuralfx_motion_overlay_pass_tests COMMAND neuralfx_motion_overlay_pass_tests)

set(NEURALFX_MOTION_PROBE_SOURCES
    src/diagnostics/motion_probe.cpp
    src/diagnostics/motion_probe_targets.cpp
    src/diagnostics/constant_dump.cpp
    src/diagnostics/probe_journal.cpp
    src/renderer/resolve_frame_pass.cpp
    src/renderer/camera_frame_slot.cpp
    src/renderer/compute_state.cpp
    src/renderer/camera_motion_pass.cpp
    src/renderer/motion_overlay_pass.cpp
)
target_sources(dxgi PRIVATE ${NEURALFX_MOTION_PROBE_SOURCES})
target_link_libraries(dxgi PRIVATE neuralfx_motion_probe_format)

add_executable(neuralfx_motion_probe_tests
    tests/motion_probe_tests.cpp
    ${NEURALFX_MOTION_PROBE_SOURCES}
    src/dxgi/context_hook.cpp
    src/dxgi/depth_capture.cpp
    src/renderer/depth_format.cpp
    src/renderer/context_state.cpp
    src/renderer/fullscreen_draw.cpp
    src/gpu/shader_compiler.cpp
    src/common/logger.cpp
)
target_compile_definitions(neuralfx_motion_probe_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_motion_probe_tests PRIVATE src)
target_link_libraries(neuralfx_motion_probe_tests PRIVATE neuralfx_temporal_d3d11 neuralfx_trace_d3d11
    neuralfx_motion_probe_format ${D3DCOMPILER_LIBRARY} user32)
add_dependencies(neuralfx_motion_probe_tests neuralfx_runtime_assets)
add_test(NAME neuralfx_motion_probe_tests COMMAND neuralfx_motion_probe_tests)
