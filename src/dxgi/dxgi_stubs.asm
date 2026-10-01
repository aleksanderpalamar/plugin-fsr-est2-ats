OPTION CASEMAP:NONE
EXTERN ResolveDxgiExport:PROC

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
name_debug db 'DXGIGetDebugInterface1',0
name_report db 'DXGIReportAdapterConfiguration',0
name_pix_begin db 'PIXBeginCapture',0
name_pix_end db 'PIXEndCapture',0
name_pix_state db 'PIXGetCaptureState',0
name_app_compat db 'SetAppCompatStringPointer',0
name_hmd db 'UpdateHMDEmulationStatus',0

.code
FORWARD MACRO symbol, name
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
    lea rcx, name
    call ResolveDxgiExport
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
END
