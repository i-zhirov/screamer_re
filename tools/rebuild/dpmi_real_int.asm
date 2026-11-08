; The rebuilt `dpmi_real_int` module.
;
; The original sits at linear 0x25db0 in STARTH.EXE and holds the real-mode
; interrupt setup: the state flags, the DOS multiplexer call INT 2Fh AX=1500h,
; the success return, and the real-mode interrupt path through INT 31h with
; the register block at `DAT_001176c0`.
;
; The module is assembly, because the original is library-style code: it holds
; `pushad`/`popad`, the direct INT 2Fh and INT 31h calls, and no EBP frame.
; The diff compares the first body, which ends at the first terminator: the
; multiplexer setup and the success return path.
.386
.model flat
public dpmi_real_int_
extrn _DAT_00116684:dword
extrn _DAT_00116688:dword
extrn _DAT_0011668c:word
extrn _DAT_0011669e:dword
extrn _DAT_001166a2:word
extrn _DAT_001166a4:dword
extrn _DAT_001166a8:dword
extrn _DAT_00116680:byte
extrn _DAT_001176c0:byte
extrn _DAT_001176d0:dword
extrn _DAT_001176d8:dword
extrn _DAT_001176dc:dword
extrn _DAT_001176e2:word
extrn error_report_:near
extrn drm_260a0_:near
extrn drm_260e0_:near
extrn drm_26014_:near
extrn drm_264ec_:near
extrn drm_262a8_:near
extrn drm_26120_:near
extrn drm_262fc_:near
.code
dpmi_real_int_ proc near
        pushad
        mov eax, 1
        mov dword ptr [_DAT_0011669e], eax
        mov dword ptr [_DAT_00116684], eax
        xor eax, eax
        xor ebx, ebx
        mov word ptr [_DAT_001166a2], ax
        mov ax, 0x1500
        int 0x2f
        test bx, bx
        jne error_path
        popad
        xor eax, eax
        mov dword ptr [_DAT_0011669e], eax
        mov word ptr [_DAT_0011668c], cx
        mov eax, 3
        ret
error_path:
        mov word ptr [_DAT_0011668c], cx
        mov eax, 0xb00
        call error_report_
        test eax, eax
        jne exit_3
        mov eax, edi
        add eax, 0x41
        mov dword ptr [_DAT_001166a8], eax
        xor eax, eax
        mov ecx, 0x30
        mov dword ptr [_DAT_001166a4], edi
zero_loop:
        mov dword ptr [edi], eax
        add edi, 4
        dec ecx
        jg zero_loop
        mov edi, dword ptr [_DAT_001166a4]
        xor ecx, ecx
        mov eax, edi
        shr edi, 4
        mov word ptr [_DAT_001176e2], di
        and eax, 0xf
        mov dword ptr [_DAT_001176d0], eax
        mov ax, 0x1501
        mov dword ptr [_DAT_001176dc], eax
        mov cx, word ptr [_DAT_0011668c]
        mov dword ptr [_DAT_001176d8], ecx
        mov bl, 0x2f
        mov ax, 0x300
        mov bh, 0
        mov cx, 0
        push ds
        pop es
        mov edi, offset _DAT_001176c0
        int 0x31
        mov edi, dword ptr [_DAT_001166a4]
        mov al, byte ptr [edi]
        mov byte ptr [_DAT_00116680], al
        mov cx, word ptr [_DAT_0011668c]
        cmp ecx, 3
        jl exit_3
        call drm_260a0_
        call drm_260e0_
        call drm_26014_
        mov edi, dword ptr [_DAT_001166a4]
        mov ax, word ptr [edi + 3]
        bt ax, 9
        jae no_hook
        call drm_264ec_
no_hook:
        xor eax, eax
        call drm_262a8_
        mov ecx, 3
poll_loop:
        call drm_260a0_
        call drm_26120_
        cmp al, 2
        jne poll_done
        dec ecx
        jne poll_loop
        xor eax, eax
        call drm_262a8_
        xor eax, eax
        mov dword ptr [_DAT_00116688], eax
        mov eax, 1
        mov dword ptr [_DAT_0011669e], eax
        popad
        mov eax, 2
        ret
poll_done:
        call drm_262fc_
        test eax, eax
        jne exit_1
        mov eax, 1
        mov dword ptr [_DAT_0011669e], eax
        popad
        xor eax, eax
        ret
exit_1:
        xor eax, eax
        call drm_262a8_
        mov eax, 1
        mov dword ptr [_DAT_0011669e], eax
        popad
        mov eax, 1
        ret
exit_3:
        popad
        xor eax, eax
        mov dword ptr [_DAT_0011669e], eax
        mov word ptr [_DAT_0011668c], cx
        mov eax, 3
        ret
dpmi_real_int_ endp
end
