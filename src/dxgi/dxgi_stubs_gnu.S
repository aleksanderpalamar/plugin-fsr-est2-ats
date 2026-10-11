.intel_syntax noprefix

.section .rdata
name_apply: .asciz "ApplyCompatResolutionQuirking"
name_compat: .asciz "CompatString"
name_compat_value: .asciz "CompatValue"
name_journal: .asciz "DXGIDumpJournal"
name_create_device: .asciz "DXGID3D10CreateDevice"
name_create_layered: .asciz "DXGID3D10CreateLayeredDevice"
name_layered_size: .asciz "DXGID3D10GetLayeredDeviceSize"
name_register_layers: .asciz "DXGID3D10RegisterLayers"
name_adapter_removal: .asciz "DXGIDeclareAdapterRemovalSupport"
name_disable_vblank: .asciz "DXGIDisableVBlankVirtualization"
name_debug: .asciz "DXGIGetDebugInterface1"
name_report: .asciz "DXGIReportAdapterConfiguration"
name_pix_begin: .asciz "PIXBeginCapture"
name_pix_end: .asciz "PIXEndCapture"
name_pix_state: .asciz "PIXGetCaptureState"
name_app_compat: .asciz "SetAppCompatStringPointer"
name_hmd: .asciz "UpdateHMDEmulationStatus"

.text
.extern ResolveDxgiExport
.extern ResolveDxgiOrdinal
.macro FORWARD symbol, name, number
.globl \symbol
.p2align 4
\symbol:
    sub rsp, 136
    mov [rsp+32], rcx
    mov [rsp+40], rdx
    mov [rsp+48], r8
    mov [rsp+56], r9
    movdqu [rsp+64], xmm0
    movdqu [rsp+80], xmm1
    movdqu [rsp+96], xmm2
    movdqu [rsp+112], xmm3
    .if \number
    mov ecx, \number
    call ResolveDxgiOrdinal
    .else
    lea rcx, [rip+\name]
    call ResolveDxgiExport
    .endif
    mov r11, rax
    mov rcx, [rsp+32]
    mov rdx, [rsp+40]
    mov r8, [rsp+48]
    mov r9, [rsp+56]
    movdqu xmm0, [rsp+64]
    movdqu xmm1, [rsp+80]
    movdqu xmm2, [rsp+96]
    movdqu xmm3, [rsp+112]
    add rsp, 136
    test r11, r11
    jz 1f
    jmp r11
1:
    mov eax, 0x80004001
    ret
.endm

FORWARD ApplyCompatResolutionQuirking, name_apply, 0
FORWARD CompatString, name_compat, 0
FORWARD CompatValue, name_compat_value, 0
FORWARD DXGIDumpJournal, name_journal, 0
FORWARD DXGID3D10CreateDevice, name_create_device, 0
FORWARD DXGID3D10CreateLayeredDevice, name_create_layered, 0
FORWARD DXGID3D10GetLayeredDeviceSize, name_layered_size, 0
FORWARD DXGID3D10RegisterLayers, name_register_layers, 0
FORWARD DXGIDeclareAdapterRemovalSupport, name_adapter_removal, 0
FORWARD DXGIDisableVBlankVirtualization, name_disable_vblank, 0
FORWARD DXGIGetDebugInterface1, name_debug, 0
FORWARD DXGIReportAdapterConfiguration, name_report, 0
FORWARD PIXBeginCapture, name_pix_begin, 0
FORWARD PIXEndCapture, name_pix_end, 0
FORWARD PIXGetCaptureState, name_pix_state, 0
FORWARD SetAppCompatStringPointer, name_app_compat, 0
FORWARD UpdateHMDEmulationStatus, name_hmd, 0
FORWARD DxgiOrdinal1, 0, 1
FORWARD DxgiOrdinal2, 0, 2
FORWARD DxgiOrdinal3, 0, 3
FORWARD DxgiOrdinal4, 0, 4
FORWARD DxgiOrdinal5, 0, 5
FORWARD DxgiOrdinal6, 0, 6
FORWARD DxgiOrdinal7, 0, 7
FORWARD DxgiOrdinal8, 0, 8
FORWARD DxgiOrdinal9, 0, 9
FORWARD DxgiOrdinal10, 0, 10
FORWARD DxgiOrdinal11, 0, 11
FORWARD DxgiOrdinal12, 0, 12
FORWARD DxgiOrdinal13, 0, 13
FORWARD DxgiOrdinal14, 0, 14
FORWARD DxgiOrdinal15, 0, 15
FORWARD DxgiOrdinal16, 0, 16
FORWARD DxgiOrdinal17, 0, 17
FORWARD DxgiOrdinal18, 0, 18
FORWARD DxgiOrdinal19, 0, 19
FORWARD DxgiOrdinal20, 0, 20
