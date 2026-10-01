# NeuralFX-ETS2

Protótipo experimental de proxy DXGI para investigar pós-processamento D3D11 no Euro Truck Simulator 2. O estado atual implementa o primeiro caminho sobre o backbuffer SDR: encaminhamento de exports DXGI conhecidos, captura de swapchains criadas por fábricas DXGI, hooks de `Present` e `ResizeBuffers`, cópia GPU, passe fullscreen, sharpening RCAS, restauração do estado D3D11, log e medição de tempo GPU. Não há validação em ETS2 1.61 ainda.

## Compilação no Windows

Requisitos: Windows x64, Visual Studio 2022 com Desktop development with C++, Windows SDK e CMake 3.24 ou superior. Execute no Developer PowerShell:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

O resultado esperado é `build/Release/dxgi.dll` com a pasta `build/Release/NeuralFX`. O shader é compilado em runtime na inicialização para facilitar os testes. A DLL usa MASM x64 para preservar os argumentos das exports encaminhadas ao DXGI do Windows.

Antes de instalar, valide os exports no mesmo Windows com `powershell -ExecutionPolicy Bypass -File tools/check-exports.ps1 build/Release/dxgi.dll`. Se o script falhar, a DLL não está pronta para esse sistema.

## Compilação cruzada no Linux

Com Zig 0.16 e Ninja:

```sh
ZIG_GLOBAL_CACHE_DIR=/tmp/neuralfx-zig-cache cmake -S . -B build/windows-x64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/zig-windows-x64.cmake -DCMAKE_BUILD_TYPE=Release
ZIG_GLOBAL_CACHE_DIR=/tmp/neuralfx-zig-cache cmake --build build/windows-x64 --parallel 4
```

O resultado fica em `build/windows-x64/dxgi.dll` com os arquivos auxiliares em `build/windows-x64/NeuralFX`. Esse caminho foi compilado como PE x64 e passou por um teste de carregamento e `CreateDXGIFactory1` no Wine. Os hooks de `Present` e `ResizeBuffers` ainda exigem teste no ETS2.

## Instalação experimental

Faça backup de qualquer `dxgi.dll` já presente no diretório do executável antes de instalar. Copie `dxgi.dll` e a pasta `NeuralFX` juntos para o diretório que contém `eurotrucks2.exe`, normalmente `bin/win_x64` dentro da instalação do ETS2. Não substitua o DXGI do Windows nem altere o executável do jogo. A confirmação de carregamento é o arquivo `NeuralFX/logs/neuralfx.log` nesse diretório.

O arquivo `NeuralFX/neuralfx.ini` contém:

```ini
enabled=false
mode=rcas
sharpness=0.65
```

O efeito começa desligado. F10 alterna o estado em runtime. `mode=hook` testa apenas o encaminhamento; `copy` executa cópia GPU ida e volta; `fullscreen` executa o passe de cópia; `rcas` aplica o sharpening. `sharpness` aceita 0 a 1. Mudanças no arquivo exigem reiniciar o jogo.


## Escopo e limitações verificadas

- O protótipo lê o backbuffer já composto; o HUD também é processado quando RCAS está ativo. O código aceita somente formatos de backbuffer RGBA/BGRA de 8 bits; a identificação do color space ativo ainda precisa de validação no jogo.
- A resolução de entrada é igual à resolução de saída. EASU está desabilitado até haver identificação confiável de um scene color target de resolução menor e separação da interface. Este protótipo não entrega upscaling FSR1.
- Não há DirectML/ONNX, overlay, captura F9, detecção de G-buffers, profundidade, motion vectors ou upscaling temporal. O sharpening implementa as equações centrais de RCAS em HLSL, com proteção numérica para as bordas.
- Os hooks cobrem swapchains criadas pelas fábricas obtidas via `CreateDXGIFactory`, `CreateDXGIFactory1` e `CreateDXGIFactory2`. Falta verificar se o caminho real de criação do ETS2 1.61 passa por essas fábricas.
- A lista de 19 exports e ordinais do proxy deve ser comparada com o `dxgi.dll` do Windows de destino antes do uso; exports adicionais e diferenças entre versões ainda não têm paridade garantida. A DLL não deve ser distribuída como substituta genérica de DXGI neste estado.

## Validação gradual no ETS2

Comece com `mode=hook`, depois `copy`, `fullscreen` e `rcas`. Em cada modo, verifique inicialização, F10, alteração de resolução, Alt+Tab, alternância fullscreen/windowed e encerramento. Confirme no log a detecção da swapchain, resolução e custo GPU. Qualquer falha na inicialização do renderer desativa o passe e encaminha `Present` ao DXGI original. Mantenha o efeito desligado para obter uma referência visual.

## Próximas investigações

BLOCKED: identificação de scene color antes do HUD e EASU. Reason: o backbuffer é a única imagem observada neste marco. Evidence: o hook atual cobre apenas criação de swapchain, `Present` e `ResizeBuffers`; não há captura de recursos internos do Prism3D. Required next investigation: captura de frame no ETS2 1.61, classificação de RTV/SRV por sequência de uso e formato, e prova de qual recurso contém a cena em resolução menor.

BLOCKED: FSR temporal e neural enhancement. Reason: depth, motion vectors, jitter e um modelo ONNX GPU validado não foram obtidos. Evidence: a referência local FidelityFX-FSR2 apresenta backends DX12/Vulkan e requer essas entradas temporais. Required next investigation: validar recursos reais do frame e depois integrar backend D3D11/DirectML compatível.

Referências: [FSR1 da AMD](https://gpuopen.com/fidelityfx-superresolution/), [código FSR1](https://github.com/GPUOpen-Effects/FidelityFX-FSR), [requisitos FSR2](https://github.com/GPUOpen-Effects/FidelityFX-FSR2), [criação de swapchain no DXGI](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgifactory-createswapchain).
