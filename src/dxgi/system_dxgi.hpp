#pragma once

#include <windows.h>

void* find_system_dxgi_export(const char* name) noexcept;
void* resolve_system_dxgi_export(const char* name) noexcept;
void* resolve_system_dxgi_ordinal(unsigned ordinal) noexcept;
