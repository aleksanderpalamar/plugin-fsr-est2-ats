add_library(neuralfx_trace_d3d11 STATIC src/diagnostics/frame_tracer.cpp
    src/diagnostics/trace_hooks.cpp src/diagnostics/trace_resources.cpp)
target_compile_features(neuralfx_trace_d3d11 PUBLIC cxx_std_20)
target_compile_definitions(neuralfx_trace_d3d11 PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_trace_d3d11 PUBLIC src)
target_link_libraries(neuralfx_trace_d3d11 PUBLIC neuralfx_diagnostics d3d11)

add_executable(neuralfx_frame_trace_hook_tests
    tests/frame_trace_hook_tests.cpp
    src/dxgi/context_hook.cpp
    src/dxgi/depth_capture.cpp
    src/renderer/depth_format.cpp
    src/common/logger.cpp
)
target_compile_definitions(neuralfx_frame_trace_hook_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(neuralfx_frame_trace_hook_tests PRIVATE neuralfx_trace_d3d11 ${D3DCOMPILER_LIBRARY} user32)
set_target_properties(neuralfx_frame_trace_hook_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests)
add_test(NAME neuralfx_frame_trace_hook_tests COMMAND neuralfx_frame_trace_hook_tests)
