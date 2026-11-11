set(NEURALFX_FSR_SDK_DIR "$ENV{NEURALFX_FSR_SDK_DIR}" CACHE PATH "Kits/FidelityFX directory of the AMD FSR SDK 2.4.0")
if(NOT NEURALFX_FSR_SDK_DIR OR NOT EXISTS "${NEURALFX_FSR_SDK_DIR}/api/include/ffx_api.h")
    message(STATUS "AMD FSR SDK not found (set NEURALFX_FSR_SDK_DIR to Kits/FidelityFX): the DX12 upscaling backend is not built")
    return()
endif()

add_library(neuralfx_upscale_dx12 STATIC
    src/upscale/dx12/ffx_runtime.cpp
    src/upscale/dx12/dx12_context.cpp
    src/upscale/dx12/shared_texture.cpp
    src/upscale/dx12/dx12_backend.cpp
)
target_compile_features(neuralfx_upscale_dx12 PUBLIC cxx_std_20)
target_compile_definitions(neuralfx_upscale_dx12 PUBLIC UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(neuralfx_upscale_dx12 PUBLIC src
    ${NEURALFX_FSR_SDK_DIR}/api/include ${NEURALFX_FSR_SDK_DIR}/upscalers/include)
target_link_libraries(neuralfx_upscale_dx12 PUBLIC neuralfx_upscale d3d11 d3d12 dxguid)

add_executable(neuralfx_dx12_backend_tests tests/dx12_backend_tests.cpp src/common/logger.cpp)
target_link_libraries(neuralfx_dx12_backend_tests PRIVATE neuralfx_upscale_dx12)
add_custom_command(TARGET neuralfx_dx12_backend_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${NEURALFX_FSR_SDK_DIR}/signedbin/amd_fidelityfx_loader_dx12.dll
        ${NEURALFX_FSR_SDK_DIR}/signedbin/amd_fidelityfx_upscaler_dx12.dll
        $<TARGET_FILE_DIR:neuralfx_dx12_backend_tests>)
add_test(NAME neuralfx_dx12_backend_tests COMMAND neuralfx_dx12_backend_tests)
set_tests_properties(neuralfx_dx12_backend_tests PROPERTIES SKIP_RETURN_CODE 77)
target_link_libraries(neuralfx_upscale_host PUBLIC neuralfx_upscale_dx12)
target_compile_definitions(neuralfx_upscale_host PRIVATE NEURALFX_HAS_FSR_DX12)
