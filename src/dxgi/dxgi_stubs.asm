OPTION CASEMAP:NONE
EXTERN ResolveDxgiExport:PROC
EXTERN ResolveDxgiOrdinal:PROC

.data
name_apply db 'ApplyCompatResolutionQuirking',0
name_compat db 'CompatString',0
name_compat_value db 'CompatValue',0
name_journal db 'DXGIDumpJournal',0
name_create_device db 'DXGID3D10CreateDevice',0
name_create_layered db 'DXGID3D10CreateLayeredDevice',0
name_layered_size db 'DXGID3D10GetLayeredDeviceSize',0
name_register_layers db 'DXGID3D10RegisterLayers',0
name_adapter_removal db 'DXGIDeclareAdapterRemovalSupport',0
name_disable_vblank db 'DXGIDisableVBlankVirtualization',0
name_debug db 'DXGIGetDebugInterface1',0
name_report db 'DXGIReportAdapterConfiguration',0
name_pix_begin db 'PIXBeginCapture',0
name_pix_end db 'PIXEndCapture',0
name_pix_state db 'PIXGetCaptureState',0
name_app_compat db 'SetAppCompatStringPointer',0
name_hmd db 'UpdateHMDEmulationStatus',0

.code
FORWARD MACRO symbol, name, number
LOCAL missing
PUBLIC symbol
symbol PROC
    sub rsp, 88h
    mov [rsp+20h], rcx
    mov [rsp+28h], rdx
    mov [rsp+30h], r8
    mov [rsp+38h], r9
    movdqu xmmword ptr [rsp+40h], xmm0
    movdqu xmmword ptr [rsp+50h], xmm1
    movdqu xmmword ptr [rsp+60h], xmm2
    movdqu xmmword ptr [rsp+70h], xmm3
    IF number EQ 0
        lea rcx, name
        call ResolveDxgiExport
    ELSE
        mov ecx, number
        call ResolveDxgiOrdinal
    ENDIF
    mov r11, rax
    mov rcx, [rsp+20h]
    mov rdx, [rsp+28h]
    mov r8, [rsp+30h]
    mov r9, [rsp+38h]
    movdqu xmm0, xmmword ptr [rsp+40h]
    movdqu xmm1, xmmword ptr [rsp+50h]
    movdqu xmm2, xmmword ptr [rsp+60h]
    movdqu xmm3, xmmword ptr [rsp+70h]
    add rsp, 88h
    test r11, r11
    jz missing
    jmp r11
missing:
    mov eax, 80004001h
    ret
symbol ENDP
ENDM

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
FORWARD DxgiOrdinal21, 0, 21
FORWARD DxgiOrdinal22, 0, 22
FORWARD DxgiOrdinal23, 0, 23
FORWARD DxgiOrdinal24, 0, 24
FORWARD DxgiOrdinal25, 0, 25
FORWARD DxgiOrdinal26, 0, 26
FORWARD DxgiOrdinal27, 0, 27
FORWARD DxgiOrdinal28, 0, 28
FORWARD DxgiOrdinal29, 0, 29
FORWARD DxgiOrdinal30, 0, 30
FORWARD DxgiOrdinal31, 0, 31
FORWARD DxgiOrdinal32, 0, 32
FORWARD DxgiOrdinal33, 0, 33
FORWARD DxgiOrdinal34, 0, 34
FORWARD DxgiOrdinal35, 0, 35
FORWARD DxgiOrdinal36, 0, 36
FORWARD DxgiOrdinal37, 0, 37
FORWARD DxgiOrdinal38, 0, 38
FORWARD DxgiOrdinal39, 0, 39
FORWARD DxgiOrdinal40, 0, 40
END
