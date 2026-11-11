add_library(neuralfx_upscale_host STATIC src/upscale/resolve_replacement.cpp src/upscale/upscale_stage.cpp
    src/upscale/upscale_runtime.cpp src/dxgi/swapchain_access.cpp
    src/renderer/upscale_inputs.cpp src/renderer/blit_pass.cpp src/renderer/depth_to_float_pass.cpp
    src/renderer/resolve_frame_pass.cpp src/renderer/camera_frame_slot.cpp src/renderer/camera_motion_pass.cpp
    src/renderer/motion_targets.cpp src/renderer/compute_state.cpp src/upscale/frame_capture.cpp src/upscale/texture_readback.cpp
    src/renderer/reactive_mask_pass.cpp)
target_compile_features(neuralfx_upscale_host PUBLIC cxx_std_20)
target_compile_definitions(neuralfx_upscale_host PUBLIC UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_upscale_host PUBLIC src)
target_link_libraries(neuralfx_upscale_host PUBLIC neuralfx_upscale neuralfx_diagnostics neuralfx_temporal neuralfx_temporal_d3d11 d3d11 ${D3DCOMPILER_LIBRARY})
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

add_executable(neuralfx_depth_to_float_pass_tests
    tests/depth_to_float_pass_tests.cpp
    src/renderer/depth_to_float_pass.cpp
    src/renderer/fullscreen_draw.cpp
    src/gpu/shader_compiler.cpp
    src/common/logger.cpp
)
target_compile_features(neuralfx_depth_to_float_pass_tests PRIVATE cxx_std_20)
target_compile_definitions(neuralfx_depth_to_float_pass_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_depth_to_float_pass_tests PRIVATE src)
target_link_libraries(neuralfx_depth_to_float_pass_tests PRIVATE neuralfx_temporal d3d11 ${D3DCOMPILER_LIBRARY})
add_dependencies(neuralfx_depth_to_float_pass_tests neuralfx_runtime_assets)
add_test(NAME neuralfx_depth_to_float_pass_tests COMMAND neuralfx_depth_to_float_pass_tests)

add_executable(neuralfx_blit_pass_tests
    tests/blit_pass_tests.cpp
    src/renderer/blit_pass.cpp
    src/renderer/fullscreen_draw.cpp
    src/gpu/shader_compiler.cpp
    src/common/logger.cpp
)
target_compile_features(neuralfx_blit_pass_tests PRIVATE cxx_std_20)
target_compile_definitions(neuralfx_blit_pass_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_blit_pass_tests PRIVATE src)
target_link_libraries(neuralfx_blit_pass_tests PRIVATE neuralfx_temporal d3d11 ${D3DCOMPILER_LIBRARY})
add_dependencies(neuralfx_blit_pass_tests neuralfx_runtime_assets)
add_test(NAME neuralfx_blit_pass_tests COMMAND neuralfx_blit_pass_tests)

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
    src/diagnostics/constant_dump.cpp
    src/diagnostics/probe_journal.cpp
    src/renderer/motion_overlay_pass.cpp
)
target_sources(dxgi PRIVATE ${NEURALFX_MOTION_PROBE_SOURCES})
target_link_libraries(dxgi PRIVATE neuralfx_motion_probe_format neuralfx_upscale_host)

add_executable(neuralfx_motion_probe_tests
    tests/motion_probe_tests.cpp
    ${NEURALFX_MOTION_PROBE_SOURCES}
    src/config/config.cpp
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
    neuralfx_motion_probe_format neuralfx_upscale_host neuralfx_config ${D3DCOMPILER_LIBRARY} user32)
add_dependencies(neuralfx_motion_probe_tests neuralfx_runtime_assets)
add_test(NAME neuralfx_motion_probe_tests COMMAND neuralfx_motion_probe_tests)

add_executable(neuralfx_upscale_stage_tests
    tests/upscale_stage_tests.cpp
    src/dxgi/context_hook.cpp
    src/dxgi/depth_capture.cpp
    src/renderer/depth_format.cpp
    src/renderer/context_state.cpp
    src/renderer/fullscreen_draw.cpp
    src/gpu/shader_compiler.cpp
    src/common/logger.cpp
)
target_compile_definitions(neuralfx_upscale_stage_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_upscale_stage_tests PRIVATE src)
target_link_libraries(neuralfx_upscale_stage_tests PRIVATE neuralfx_upscale_host neuralfx_temporal_d3d11
    neuralfx_trace_d3d11 ${D3DCOMPILER_LIBRARY} user32)
add_dependencies(neuralfx_upscale_stage_tests neuralfx_runtime_assets)
add_test(NAME neuralfx_upscale_stage_tests COMMAND neuralfx_upscale_stage_tests)

add_executable(neuralfx_upscale_stage_oversized_tests
    tests/upscale_stage_oversized_tests.cpp
    src/dxgi/context_hook.cpp
    src/dxgi/depth_capture.cpp
    src/renderer/depth_format.cpp
    src/renderer/context_state.cpp
    src/renderer/fullscreen_draw.cpp
    src/gpu/shader_compiler.cpp
    src/common/logger.cpp
)
target_compile_definitions(neuralfx_upscale_stage_oversized_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_upscale_stage_oversized_tests PRIVATE src)
target_link_libraries(neuralfx_upscale_stage_oversized_tests PRIVATE neuralfx_upscale_host neuralfx_temporal_d3d11
    neuralfx_trace_d3d11 ${D3DCOMPILER_LIBRARY} user32)
add_dependencies(neuralfx_upscale_stage_oversized_tests neuralfx_runtime_assets)
add_test(NAME neuralfx_upscale_stage_oversized_tests COMMAND neuralfx_upscale_stage_oversized_tests)

add_executable(neuralfx_reactive_mask_pass_tests
    tests/reactive_mask_pass_tests.cpp
    src/renderer/reactive_mask_pass.cpp
    src/renderer/fullscreen_draw.cpp
    src/gpu/shader_compiler.cpp
    src/common/logger.cpp
)
target_compile_features(neuralfx_reactive_mask_pass_tests PRIVATE cxx_std_20)
target_compile_definitions(neuralfx_reactive_mask_pass_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_reactive_mask_pass_tests PRIVATE src)
target_link_libraries(neuralfx_reactive_mask_pass_tests PRIVATE neuralfx_temporal d3d11 ${D3DCOMPILER_LIBRARY})
add_dependencies(neuralfx_reactive_mask_pass_tests neuralfx_runtime_assets)
add_test(NAME neuralfx_reactive_mask_pass_tests COMMAND neuralfx_reactive_mask_pass_tests)

add_executable(neuralfx_upscale_stage_auto_capture_tests
    tests/upscale_stage_auto_capture_tests.cpp
    src/dxgi/context_hook.cpp
    src/dxgi/depth_capture.cpp
    src/renderer/depth_format.cpp
    src/renderer/context_state.cpp
    src/renderer/fullscreen_draw.cpp
    src/gpu/shader_compiler.cpp
    src/common/logger.cpp
)
target_compile_definitions(neuralfx_upscale_stage_auto_capture_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_upscale_stage_auto_capture_tests PRIVATE src)
target_link_libraries(neuralfx_upscale_stage_auto_capture_tests PRIVATE neuralfx_upscale_host neuralfx_temporal_d3d11
    neuralfx_trace_d3d11 ${D3DCOMPILER_LIBRARY} user32)
add_dependencies(neuralfx_upscale_stage_auto_capture_tests neuralfx_runtime_assets)
add_test(NAME neuralfx_upscale_stage_auto_capture_tests COMMAND neuralfx_upscale_stage_auto_capture_tests)
