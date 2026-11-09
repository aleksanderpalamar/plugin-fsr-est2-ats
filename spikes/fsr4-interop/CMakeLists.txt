cmake_minimum_required(VERSION 3.24)
project(NeuralFX_FSR4_Interop_Spike LANGUAGES CXX)

set(FSR_SDK_DIR "" CACHE PATH "Kits/FidelityFX directory of the AMD FSR SDK v2.4.0")
if(NOT EXISTS "${FSR_SDK_DIR}/api/include/ffx_api.h")
    message(FATAL_ERROR "Set FSR_SDK_DIR to the Kits/FidelityFX directory of the FSR SDK")
endif()

add_executable(fsr4_interop_probe
    main.cpp
    d3d11_source.cpp
    d3d12_sink.cpp
    d3d12_readback.cpp
    fsr_probe.cpp
)
target_compile_features(fsr4_interop_probe PRIVATE cxx_std_20)
target_compile_definitions(fsr4_interop_probe PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(fsr4_interop_probe PRIVATE -Wall -Wextra)
target_include_directories(fsr4_interop_probe PRIVATE
    ${FSR_SDK_DIR}/api/include ${FSR_SDK_DIR}/upscalers/include)
target_link_libraries(fsr4_interop_probe PRIVATE d3d11 d3d12 dxgi dxguid)
add_custom_command(TARGET fsr4_interop_probe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${FSR_SDK_DIR}/signedbin/amd_fidelityfx_loader_dx12.dll
        ${FSR_SDK_DIR}/signedbin/amd_fidelityfx_upscaler_dx12.dll
        $<TARGET_FILE_DIR:fsr4_interop_probe>)

set(VULKAN_HEADERS_DIR "" CACHE PATH "Directory containing the vulkan/ and vk_video/ header folders")
if(EXISTS "${VULKAN_HEADERS_DIR}/vulkan/vulkan.h")
    add_executable(fsr4_vulkan_probe vk_probe.cpp vulkan_device.cpp)
    target_compile_features(fsr4_vulkan_probe PRIVATE cxx_std_20)
    target_compile_definitions(fsr4_vulkan_probe PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX
        VK_USE_PLATFORM_WIN32_KHR)
    target_compile_options(fsr4_vulkan_probe PRIVATE -Wall -Wextra)
    target_include_directories(fsr4_vulkan_probe SYSTEM PRIVATE ${VULKAN_HEADERS_DIR})
    target_include_directories(fsr4_vulkan_probe PRIVATE
        ${FSR_SDK_DIR}/api/include ${FSR_SDK_DIR}/upscalers/include)
    add_custom_command(TARGET fsr4_vulkan_probe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FSR_SDK_DIR}/signedbin/amd_fidelityfx_loader_vk.dll
            ${FSR_SDK_DIR}/signedbin/amd_fidelityfx_upscaler_vk.dll
            $<TARGET_FILE_DIR:fsr4_vulkan_probe>)
endif()
