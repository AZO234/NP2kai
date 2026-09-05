; NP2IDLE.ASM
; ---------------------------------------------------------------------------
; NP2IDLE - Windows 95/98/Me idle HLT helper for Neko Project 21/W
;
; Build target:
;   Microsoft Windows 98 DDK + MASM
;
; WIN40COMPAT is defined before the DDK include files so that a VxD built
; with the Windows 98 DDK can still be loaded by Windows 95.
;
; Ring-3 control interface:
;   Device name: \\.\NP2IDLE0
;   IOCTL 1     : enable HLT idle handling
;   IOCTL 2     : disable HLT idle handling
;   IOCTL 3     : query state (DWORD 0/1 in output buffer)
;
; The device name deliberately has eight characters.  This avoids old
; Windows 95 CreateFile/VxD-name compatibility problems seen with some
; shorter DDB names.  The VxD file itself remains NP2IDLE.VXD.
; ---------------------------------------------------------------------------

.386p

WIN40COMPAT EQU 1

INCLUDE VMM.INC
INCLUDE VWIN32.INC

; Private DeviceIoControl codes.  0 is reserved for DIOC_OPEN/GETVERSION
; and -1 for DIOC_CLOSEHANDLE, so private commands begin at 1.
NP2IDLE_DIOC_ENABLE       EQU 1
NP2IDLE_DIOC_DISABLE      EQU 2
NP2IDLE_DIOC_GETSTATE     EQU 3

; Win32 error values returned in EAX by W32_DEVICEIOCONTROL.
NP2_ERR_INVALID_FUNCTION    EQU 1
NP2_ERR_INSUFFICIENT_BUFFER EQU 122

; Keep the ring-3-visible DDB/device name at exactly eight characters.
Declare_Virtual_Device NP2IDLE0, 1, 1, NP2IDLE_Control, \
                       Undefined_Device_ID, Undefined_Init_Order


; ---------------------------------------------------------------------------
; Locked data
; ---------------------------------------------------------------------------

VxD_LOCKED_DATA_SEG

        ALIGN 4
NP2IDLE_Enabled        dd 1        ; default: enabled

VxD_LOCKED_DATA_ENDS


; ---------------------------------------------------------------------------
; Locked code
; ---------------------------------------------------------------------------

VxD_LOCKED_CODE_SEG

BeginProc NP2IDLE_Control

        Control_Dispatch Device_Init,          NP2IDLE_Device_Init
        Control_Dispatch W32_DEVICEIOCONTROL, NP2IDLE_W32_DeviceIoControl

        clc
        ret

EndProc NP2IDLE_Control


; ---------------------------------------------------------------------------
; NP2IDLE_Idle_Callback
;
; Called by VMM when Windows and all VMs are idle.
;
; We leave the callback permanently registered.  ON/OFF only changes the
; aligned DWORD above, which avoids registration/cancellation races.
;
; Return CF=1 so that later idle callbacks still receive the idle indication.
; ---------------------------------------------------------------------------

BeginProc NP2IDLE_Idle_Callback

        cmp     dword ptr [NP2IDLE_Enabled], 0
        je      SHORT NP2IDLE_No_Halt

        ; Never force STI here.  If VMM entered with hardware interrupts
        ; disabled, skip HLT rather than changing VMM interrupt state.
        pushfd
        pop     eax
        test    eax, 00000200h             ; EFLAGS.IF
        jz      SHORT NP2IDLE_No_Halt

        hlt                                ; wake on next interrupt

NP2IDLE_No_Halt:
        stc                                ; continue idle-handler chain
        ret

EndProc NP2IDLE_Idle_Callback


; ---------------------------------------------------------------------------
; NP2IDLE_W32_DeviceIoControl
;
; ESI -> DIOCParams (defined by VWIN32.INC)
;
; Successful W32_DEVICEIOCONTROL requests return ERROR_SUCCESS (EAX=0).
; Unsupported/bad requests return a Win32 error value in EAX.  Carry is
; cleared on return as required by the Win9x DeviceIoControl protocol.
; ---------------------------------------------------------------------------

BeginProc NP2IDLE_W32_DeviceIoControl

        ASSUME  esi:PTR DIOCParams

        ; DeviceIoControl normally supplies this pointer.  Clear it first
        ; when present, so commands with no output report zero bytes.
        mov     edx, [esi].lpcbBytesReturned
        test    edx, edx
        jz      SHORT NP2IDLE_DIOC_NoByteCount
        mov     dword ptr [edx], 0

NP2IDLE_DIOC_NoByteCount:
        mov     eax, [esi].dwIoControlCode

        ; CreateFile("\\\\.\\NP2IDLE0") probes the VxD with DIOC_OPEN.
        cmp     eax, DIOC_OPEN
        je      SHORT NP2IDLE_DIOC_Success

        ; CloseHandle generates this notification.  No per-handle state exists.
        cmp     eax, DIOC_CLOSEHANDLE
        je      SHORT NP2IDLE_DIOC_Success

        cmp     eax, NP2IDLE_DIOC_ENABLE
        je      SHORT NP2IDLE_DIOC_EnableJmp

        cmp     eax, NP2IDLE_DIOC_DISABLE
        je      SHORT NP2IDLE_DIOC_DisableJmp

        cmp     eax, NP2IDLE_DIOC_GETSTATE
        je      SHORT NP2IDLE_DIOC_GetStateJmp

        mov     eax, NP2_ERR_INVALID_FUNCTION
        clc
        ret

NP2IDLE_DIOC_EnableJmp:
        mov     dword ptr [NP2IDLE_Enabled], 1
        jmp     SHORT NP2IDLE_DIOC_Success

NP2IDLE_DIOC_DisableJmp:
        mov     dword ptr [NP2IDLE_Enabled], 0
        jmp     SHORT NP2IDLE_DIOC_Success

NP2IDLE_DIOC_GetStateJmp:
        ; STATUS requires a DWORD output buffer.
        cmp     [esi].cbOutBuffer, 4
        jb      SHORT NP2IDLE_DIOC_SmallBuffer

        mov     edi, [esi].lpvOutBuffer
        test    edi, edi
        jz      SHORT NP2IDLE_DIOC_SmallBuffer

        mov     eax, [NP2IDLE_Enabled]
        and     eax, 1
        mov     dword ptr [edi], eax

        mov     edx, [esi].lpcbBytesReturned
        test    edx, edx
        jz      SHORT NP2IDLE_DIOC_Success
        mov     dword ptr [edx], 4
        jmp     SHORT NP2IDLE_DIOC_Success

NP2IDLE_DIOC_SmallBuffer:
        mov     eax, NP2_ERR_INSUFFICIENT_BUFFER
        clc
        ret

NP2IDLE_DIOC_Success:
        xor     eax, eax                  ; ERROR_SUCCESS
        clc
        ret

        ASSUME  esi:NOTHING

EndProc NP2IDLE_W32_DeviceIoControl

VxD_LOCKED_CODE_ENDS


; ---------------------------------------------------------------------------
; Initialization code
; ---------------------------------------------------------------------------

VxD_INIT_CODE_SEG

BeginProc NP2IDLE_Device_Init

        mov     esi, OFFSET32 NP2IDLE_Idle_Callback
        VMMcall Call_When_Idle

        ; Preserve Call_When_Idle's CF result.  Initialization fails if VMM
        ; could not register the callback.
        ret

EndProc NP2IDLE_Device_Init

VxD_INIT_CODE_ENDS

END
