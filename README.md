# NeuralFX-ETS2

Version 2.0.0 is a DXGI proxy for D3D11 post-processing in Euro Truck Simulator 2 and American Truck Simulator. It applies screen-space reflections and approximate indirect lighting to the game's SDR image using a captured depth buffer, followed by photorealistic grading, the included LUT, and RCAS sharpening. The ray tracing mode was visually tested in ETS2 1.61.1.1 via Proton; ATS still needs an in-game test of this mode.

## Building on Windows

Requirements: Windows x64, Visual Studio 2022 with Desktop development with C++, the Windows SDK, and CMake 3.24 or newer. Run these commands in Developer PowerShell:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target neuralfx_package
```

The expected output is `build/Release/dxgi.dll` alongside the `build/Release/NeuralFX` folder. By default, the installation package is written to `dist/FSR-ets2-ats-2.0.0.zip`, using `project(... VERSION ...)` in CMake. Shaders are compiled at runtime during initialization to make testing easier. The DLL uses x64 MASM to preserve the arguments of exports forwarded to Windows DXGI.

Before installing, validate the exports on the same Windows system with `powershell -ExecutionPolicy Bypass -File tools/check-exports.ps1 build/Release/dxgi.dll`. If the script fails, the DLL is not ready for that system.

## Cross-compiling on Linux

With Zig 0.16, Ninja, and CMake, run these scripts from the repository root. The build script compiles the project; the package script builds it and creates the installation ZIP:

```sh
./scripts/build.sh
./scripts/package.sh 2.0.3
```

Pass the desired `major.minor.patch` version to `package.sh`. It writes `dist/FSR-ets2-ats-<version>.zip` and prints the full path. Without an argument, it uses the project version from CMake. The output is `build/windows-x64/dxgi.dll` with supporting files in `build/windows-x64/NeuralFX`. This build was compiled as PE x64, passed the proxy smoke test in Wine, and ran in ETS2 1.61.1.1 via Proton with the ray tracing mode enabled.

## Pull request quality gate

Pull requests targeting `main` run the [PR quality gate](.github/workflows/pr-quality-gate.yml). Linux checks shell syntax, compiler warnings, native unit tests, the Zig cross-build, and the package contents. Windows builds the D3D11 proxy with warnings treated as errors, compiles the shaders, and runs the configuration and DXGI proxy smoke tests.

Every automated check must pass. Changes to deterministic configuration or LUT rules need unit coverage; shader changes must compile all entry points; proxy changes must pass the DXGI smoke test. Rendering changes also need in-game verification in ETS2 and ATS because CI cannot exercise in-game behavior.

To make the gate mandatory, configure a [GitHub branch ruleset](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches) for `main` that requires pull requests and the `Linux validation` and `Windows validation` status checks.

## Installation

Back up any existing `dxgi.dll` in the executable directory before installing. Copy `dxgi.dll` and the `NeuralFX` folder together into the directory containing `eurotrucks2.exe`, usually `bin/win_x64` within the ETS2 installation. Do not replace Windows DXGI or modify the game executable. The `NeuralFX/logs/neuralfx.log` file in that directory confirms that the plugin loaded.

The distributed `NeuralFX/neuralfx.ini` uses:

```ini
enabled=true
mode=raytracing
photoreal_sharpness=0.30
lut_strength=0.5
lut_path=luts/neuralfx-cool.cube
```

The included, project-generated LUT is located at `NeuralFX/luts/neuralfx-cool.cube`. `mode=hook` tests forwarding only; `copy` performs a GPU copy round trip; `fullscreen` runs the copy pass; `rcas` applies sharpening; and `photoreal` runs grading, RCAS, and finishing. `mode=raytracing` adds screen-space reflections and approximate indirect lighting from the game's color and depth buffers before the photoreal, LUT, and RCAS passes. It can only use geometry visible in the depth buffer; if the buffer is unavailable, it falls back to the photoreal pipeline. `sharpness` accepts values from 0 to 1 in `rcas` mode; `photoreal_sharpness` controls RCAS in `photoreal` and `raytracing` modes. Changing `mode` requires restarting the game.

In `photoreal` and `raytracing` modes, the backbuffer is copied before `Present`. Ray tracing adds a screen-space pass before color processing. The color pass writes sRGB to an `R16G16B16A16_FLOAT` intermediate target; the finishing pass applies RCAS, grain, and dither to the backbuffer. The `exposure_ev`, `contrast`, `saturation`, `clarity`, `highlight_boost`, `highlight_warmth`, `shadow_coolness`, `neural_strength`, `black_level`, `lut_strength`, `grain_strength`, `tone_strength`, `highlight_start`, `highlight_end`, and `photoreal_sharpness` parameters can be changed in the INI while the game is running; the plugin reloads them within one second. The initial `photoreal_sharpness` value is 0.30. `lut_path` points to a 3D `.cube` LUT with a 0-to-1 domain and a size from 2 to 64, relative to the `NeuralFX` folder. The LUT is optional and is applied only when `lut_strength` is greater than zero. The neural residual is optional and is applied only when a producer supplies an SRV to the renderer and `neural_strength` is greater than zero; this version does not include such a producer.

## Verified scope and limitations

- The plugin reads the already composed backbuffer, so post-processing also affects the HUD. The code accepts only 8-bit RGBA/BGRA backbuffer formats. For `_SRGB` formats, the SRV decodes and the RTV encodes sRGB in hardware. For `_UNORM` formats, the shader assumes sRGB-encoded SDR values, decodes them for grading, and writes encoded values after RCAS. The actual ETS2 color space still needs validation through a frame capture.
- HDR10 and scRGB are outside the supported path because their backbuffer formats are rejected during initialization. The image is left unchanged in those modes. The game already applies its own tone mapping before this hook, so the filmic tone mapper is blended subtly using `tone_strength`, initially 0.20.
- Input and output resolutions are the same. EASU is disabled until a lower-resolution scene color target can be identified reliably and the HUD can be separated. This version does not provide FSR1 upscaling.
- The plugin captures a depth target for screen-space ray tracing. There is no DirectML/ONNX, overlay, F9 capture, G-buffer detection, motion vectors, or temporal upscaling. Sharpening implements the core RCAS equations in HLSL, with numerical safeguards at the edges.
- The hooks cover swapchains created by factories obtained through `CreateDXGIFactory`, `CreateDXGIFactory1`, and `CreateDXGIFactory2`. Swapchain creation and depth capture were observed in ETS2 1.61.1.1 via Proton.
- The proxy's list of 19 exports and ordinals must be compared with the `dxgi.dll` on the target Windows system before use. Additional exports and differences between versions do not yet have guaranteed parity. At this stage, the DLL should not be distributed as a general-purpose DXGI replacement.

## Step-by-step validation in ETS2

Start with `mode=hook`, then try `copy`, `fullscreen`, and `rcas`. In each mode, check initialization, resolution changes, Alt+Tab, switching between fullscreen and windowed modes, and shutdown. Confirm swapchain detection, resolution, and GPU cost in the log. If renderer initialization fails, the pass is disabled and `Present` is forwarded to the original DXGI. Keep the effect disabled to obtain a visual reference.

## Next investigations

BLOCKED: identifying scene color before the HUD and enabling EASU. Reason: the plugin currently uses the composed backbuffer for color. It captures a depth target for screen-space ray tracing but does not identify the game's internal scene color target. Required next investigation: classify RTVs and SRVs by usage sequence and format, and establish which resource contains the lower-resolution scene.

BLOCKED: temporal FSR and neural enhancement. Reason: although depth is captured for screen-space tracing, motion vectors, jitter, and a validated GPU ONNX model have not been obtained. Evidence: the local FidelityFX-FSR2 reference provides DX12/Vulkan backends and requires those temporal inputs. Required next investigation: validate actual frame resources, then integrate a compatible D3D11/DirectML backend.

References: [AMD FSR1](https://gpuopen.com/fidelityfx-superresolution/), [FSR1 source code](https://github.com/GPUOpen-Effects/FidelityFX-FSR), [FSR2 requirements](https://github.com/GPUOpen-Effects/FidelityFX-FSR2), [DXGI swapchain creation](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgifactory-createswapchain).
