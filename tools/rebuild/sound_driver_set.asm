; The rebuilt `sound_driver_set` module.
;
; The original sits at linear 0x262a8 in STARTH.EXE and holds 17 compared
; instructions and 58 bytes: `pushad`, the two state guards, the HMI request
; block built at `DAT_001166a4`, the call at `0x25f24`, and the epilogue.
;
; The module builds the HMI request block from the argument and the state
; words, and runs the sound driver through the call when the driver state is
; set.
;
; The module is assembly, because the original is library-style code: it holds
; `pushad`/`popad` and no EBP frame.
.386
.model flat
public sound_driver_set_
extrn _DAT_00116684:dword
extrn _DAT_0011668c:word
extrn _DAT_0011669a:dword
extrn _DAT_001166a4:dword
extrn _DAT_00116680:byte
extrn sd_setup_:near
.code
sound_driver_set_ proc near
        pushad
        mov bx, word ptr [_DAT_0011668c]
        or bx, bx
        je quiet
        bt dword ptr [_DAT_00116684], 0
        jae quiet
        mov dword ptr [_DAT_0011669a], eax
        mov edi, dword ptr [_DAT_001166a4]
        mov byte ptr [edi + 0x41], al
        xor ecx, ecx
        mov al, 0x1a
        mov byte ptr [edi], al
        mov al, 0xc
        mov byte ptr [edi + 2], al
        mov al, byte ptr [_DAT_00116680]
        mov byte ptr [edi + 1], al
        mov ax, 2
        mov word ptr [edi + 0x12], ax
        mov al, 1
        mov byte ptr [edi + 0x40], al
        mov dword ptr [edi + 0x42], ecx
        call sd_setup_
        popad
        ret
quiet:
        popad
        ret
sound_driver_set_ endp
end
