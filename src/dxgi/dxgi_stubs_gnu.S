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
name_debug: .asciz "DXGIGetDebugInterface1"
name_report: .asciz "DXGIReportAdapterConfiguration"
name_pix_begin: .asciz "PIXBeginCapture"
name_pix_end: .asciz "PIXEndCapture"
name_pix_state: .asciz "PIXGetCaptureState"
name_app_compat: .asciz "SetAppCompatStringPointer"
name_hmd: .asciz "UpdateHMDEmulationStatus"

.text
.extern ResolveDxgiExport
.macro FORWARD symbol, name
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
    lea rcx, [rip+\name]
    call ResolveDxgiExport
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

FORWARD ApplyCompatResolutionQuirking, name_apply
FORWARD CompatString, name_compat
FORWARD CompatValue, name_compat_value
FORWARD DXGIDumpJournal, name_journal
FORWARD DXGID3D10CreateDevice, name_create_device
FORWARD DXGID3D10CreateLayeredDevice, name_create_layered
FORWARD DXGID3D10GetLayeredDeviceSize, name_layered_size
FORWARD DXGID3D10RegisterLayers, name_register_layers
FORWARD DXGIDeclareAdapterRemovalSupport, name_adapter_removal
FORWARD DXGIGetDebugInterface1, name_debug
FORWARD DXGIReportAdapterConfiguration, name_report
FORWARD PIXBeginCapture, name_pix_begin
FORWARD PIXEndCapture, name_pix_end
FORWARD PIXGetCaptureState, name_pix_state
FORWARD SetAppCompatStringPointer, name_app_compat
FORWARD UpdateHMDEmulationStatus, name_hmd
