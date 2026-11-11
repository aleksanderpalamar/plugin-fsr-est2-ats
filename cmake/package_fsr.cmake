if(NOT TARGET neuralfx_upscale_dx12)
    return()
endif()

add_custom_target(neuralfx_package_fsr
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/dist
    COMMAND ${CMAKE_COMMAND}
        "-DNEURALFX_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DNEURALFX_OUTPUT=$<TARGET_FILE_DIR:dxgi>/README"
        "-DNEURALFX_PACKAGE_VERSION=${NEURALFX_PACKAGE_VERSION}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/write_package_readme.cmake"
    COMMAND ${CMAKE_COMMAND} "-DSDK=${NEURALFX_FSR_SDK_DIR}" "-DOUT=$<TARGET_FILE_DIR:dxgi>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/stage_fsr_files.cmake"
    COMMAND ${CMAKE_COMMAND} -E tar cf
        ${CMAKE_CURRENT_SOURCE_DIR}/dist/${NEURALFX_PACKAGE_NAME}-fsr.zip --format=zip
        $<TARGET_FILE_NAME:dxgi> NeuralFX/neuralfx.ini NeuralFX/shaders
        NeuralFX/luts/neuralfx-ets2.cube NeuralFX/luts/neuralfx-ats.cube
        NeuralFX/luts/neuralfx-ets2-profile-2.cube README
        amd_fidelityfx_loader_dx12.dll amd_fidelityfx_upscaler_dx12.dll
        NeuralFX/AMD_FIDELITYFX_LICENSE.md NeuralFX/fsr-manifest.txt
    COMMAND ${CMAKE_COMMAND} -E echo
        "FSR package generated: ${CMAKE_CURRENT_SOURCE_DIR}/dist/${NEURALFX_PACKAGE_NAME}-fsr.zip"
    WORKING_DIRECTORY $<TARGET_FILE_DIR:dxgi>
    DEPENDS neuralfx_runtime_assets
    VERBATIM
)
