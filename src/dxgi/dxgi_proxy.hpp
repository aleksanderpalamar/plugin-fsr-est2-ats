#pragma once

#include <windows.h>

extern "C" void* __cdecl ResolveDxgiExport(const char* name) noexcept;
extern "C" void* __cdecl ResolveDxgiOrdinal(unsigned ordinal) noexcept;
extern "C" HRESULT WINAPI CreateDXGIFactory2(UINT flags, REFIID iid, void** output);
