set(loader "${SDK}/signedbin/amd_fidelityfx_loader_dx12.dll")
set(upscaler "${SDK}/signedbin/amd_fidelityfx_upscaler_dx12.dll")
set(license "${SDK}/docs/license.md")
foreach(required IN ITEMS "${loader}" "${upscaler}" "${license}")
    if(NOT EXISTS "${required}")
        message(FATAL_ERROR "Missing AMD FidelityFX SDK file: ${required}")
    endif()
endforeach()
file(COPY "${loader}" "${upscaler}" DESTINATION "${OUT}")
file(MAKE_DIRECTORY "${OUT}/NeuralFX")
file(COPY_FILE "${license}" "${OUT}/NeuralFX/AMD_FIDELITYFX_LICENSE.md")
file(SHA256 "${loader}" loader_hash)
file(SHA256 "${upscaler}" upscaler_hash)
file(WRITE "${OUT}/NeuralFX/fsr-manifest.txt"
"AMD FidelityFX SDK 2.4.0 signed binaries, unmodified, x64\n"
"amd_fidelityfx_loader_dx12.dll ${loader_hash}\n"
"amd_fidelityfx_upscaler_dx12.dll ${upscaler_hash}\n"
"License: NeuralFX/AMD_FIDELITYFX_LICENSE.md\n")
