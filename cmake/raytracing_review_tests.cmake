add_executable(neuralfx_raytracing_review_tests
    tests/raytracing_review_tests.cpp
    src/renderer/screen_space_pass.cpp
    src/renderer/fullscreen_draw.cpp
    src/gpu/shader_compiler.cpp
    src/common/logger.cpp
)
target_compile_features(neuralfx_raytracing_review_tests PRIVATE cxx_std_20)
target_compile_definitions(neuralfx_raytracing_review_tests PRIVATE
    UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_raytracing_review_tests PRIVATE src)
target_link_libraries(neuralfx_raytracing_review_tests PRIVATE d3d11 ${D3DCOMPILER_LIBRARY})
set_target_properties(neuralfx_raytracing_review_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/raytracing-review)
add_custom_target(neuralfx_raytracing_review_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:neuralfx_raytracing_review_tests>/NeuralFX/shaders
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/tests/assets/raytracing_review.hlsl
        $<TARGET_FILE_DIR:neuralfx_raytracing_review_tests>/NeuralFX/shaders/raytracing_review.hlsl
    DEPENDS neuralfx_raytracing_review_tests
    VERBATIM
)
add_test(NAME neuralfx_raytracing_review_tests COMMAND neuralfx_raytracing_review_tests)
