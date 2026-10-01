#include <windows.h>
#include <dxgi1_2.h>

#include <cstdio>

int main(int argument_count, char** arguments) {
    if (argument_count != 2) return 1;
    HMODULE proxy = LoadLibraryA(arguments[1]);
    if (!proxy) {
        std::fprintf(stderr, "LoadLibrary failed: %lu\n", GetLastError());
        return 2;
    }
    constexpr const char* exports[] = {
        "ApplyCompatResolutionQuirking", "CompatString", "CompatValue",
        "DXGIDumpJournal", "PIXBeginCapture", "PIXEndCapture",
        "PIXGetCaptureState", "SetAppCompatStringPointer",
        "UpdateHMDEmulationStatus", "CreateDXGIFactory",
        "CreateDXGIFactory1", "CreateDXGIFactory2",
        "DXGID3D10CreateDevice", "DXGID3D10CreateLayeredDevice",
        "DXGID3D10GetLayeredDeviceSize", "DXGID3D10RegisterLayers",
        "DXGIDeclareAdapterRemovalSupport", "DXGIGetDebugInterface1",
        "DXGIReportAdapterConfiguration"
    };
    for (const char* name : exports) {
        if (GetProcAddress(proxy, name)) continue;
        std::fprintf(stderr, "Missing export: %s\n", name);
        return 3;
    }
    using CreateFactory = HRESULT(WINAPI*)(REFIID, void**);
    auto create = reinterpret_cast<CreateFactory>(GetProcAddress(proxy, "CreateDXGIFactory1"));
    void* output = nullptr;
    HRESULT result = create(IID_IDXGIFactory1, &output);
    if (FAILED(result) || !output) {
        std::fprintf(stderr, "CreateDXGIFactory1 failed: 0x%08lx\n", static_cast<unsigned long>(result));
        return 4;
    }
    static_cast<IDXGIFactory1*>(output)->Release();
    FreeLibrary(proxy);
    return 0;
}
