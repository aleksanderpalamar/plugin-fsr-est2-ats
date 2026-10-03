cmake_minimum_required(VERSION 3.24)
project(NeuralFX_ETS2 VERSION 1.0.0 LANGUAGES CXX)

add_library(neuralfx_config STATIC src/config/config_parser.cpp src/renderer/lut_data.cpp)
target_compile_features(neuralfx_config PUBLIC cxx_std_20)
target_include_directories(neuralfx_config PUBLIC src)
enable_testing()
add_executable(neuralfx_config_tests tests/config_tests.cpp)
target_link_libraries(neuralfx_config_tests PRIVATE neuralfx_config)
add_test(NAME neuralfx_config_tests COMMAND neuralfx_config_tests)

if(NOT WIN32)
    return()
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "NeuralFX requires a Windows x64 toolchain")
endif()

if(MSVC)
    enable_language(ASM_MASM)
    set(DXGI_STUB_SOURCE src/dxgi/dxgi_stubs.asm)
    set(D3DCOMPILER_LIBRARY d3dcompiler)
else()
    enable_language(ASM)
    set(DXGI_STUB_SOURCE src/dxgi/dxgi_stubs_gnu.S)
    set(D3DCOMPILER_LIBRARY d3dcompiler_47)
endif()

add_library(dxgi SHARED
    src/dllmain.cpp
    src/dxgi/dxgi_proxy.cpp
    ${DXGI_STUB_SOURCE}
    src/dxgi/dxgi.def
    src/dxgi/swapchain_hook.cpp
    src/renderer/renderer.cpp
    src/renderer/renderer_passes.cpp
    src/renderer/resource_manager.cpp
    src/renderer/lut_resource.cpp
    src/renderer/context_state.cpp
    src/renderer/gpu_profiler.cpp
    src/common/logger.cpp
    src/config/config.cpp
)

target_compile_features(dxgi PRIVATE cxx_std_20)
target_compile_definitions(dxgi PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
if(MSVC)
    target_compile_options(dxgi PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(dxgi PRIVATE -Wall -Wextra)
endif()
target_include_directories(dxgi PRIVATE src)
target_link_libraries(dxgi PRIVATE neuralfx_config d3d11 ${D3DCOMPILER_LIBRARY} dxguid user32 kernel32)
set_target_properties(dxgi PROPERTIES OUTPUT_NAME dxgi PREFIX "")

add_executable(neuralfx_dxgi_smoke tests/dxgi_proxy_smoke.cpp)
target_compile_features(neuralfx_dxgi_smoke PRIVATE cxx_std_20)
target_link_libraries(neuralfx_dxgi_smoke PRIVATE dxguid kernel32)
add_dependencies(neuralfx_dxgi_smoke dxgi)
add_test(NAME neuralfx_dxgi_smoke COMMAND neuralfx_dxgi_smoke $<TARGET_FILE:dxgi>)

add_executable(neuralfx_shader_compile_tests tests/shader_compile.cpp)
target_compile_features(neuralfx_shader_compile_tests PRIVATE cxx_std_20)
target_link_libraries(neuralfx_shader_compile_tests PRIVATE ${D3DCOMPILER_LIBRARY})
add_test(NAME neuralfx_shader_compile_tests COMMAND neuralfx_shader_compile_tests)

add_custom_target(neuralfx_runtime_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:dxgi>/NeuralFX/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/luts $<TARGET_FILE_DIR:dxgi>/NeuralFX/luts
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/config/neuralfx.ini $<TARGET_FILE_DIR:dxgi>/NeuralFX/neuralfx.ini
    DEPENDS dxgi
)
add_dependencies(neuralfx_shader_compile_tests neuralfx_runtime_assets)

set(NEURALFX_PACKAGE_NAME FSR-ets2-ats-${PROJECT_VERSION})
add_custom_target(neuralfx_package
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/dist
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/README $<TARGET_FILE_DIR:dxgi>/README
    COMMAND ${CMAKE_COMMAND} -E tar cf
        ${CMAKE_CURRENT_SOURCE_DIR}/dist/${NEURALFX_PACKAGE_NAME}.zip --format=zip
        $<TARGET_FILE_NAME:dxgi> NeuralFX/neuralfx.ini NeuralFX/shaders
        NeuralFX/luts README
    WORKING_DIRECTORY $<TARGET_FILE_DIR:dxgi>
    DEPENDS neuralfx_runtime_assets
    VERBATIM
)
