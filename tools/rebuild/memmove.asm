; The rebuilt `memmove` module.
;
; The original sits at linear 0xd2066 in STARTH.EXE and holds 23 instructions
; and 37 bytes: the saves of `ecx`, `esi`, `edi`, and `es`, the destination
; pointer in `edi`, the source pointer in `esi`, the byte count in `ecx`, the
; `es` segment set to `ds`, the `repne movsd` of the whole dwords, the
; `repne movsb` of the remainder bytes, the return of the destination pointer
; in `eax`, and the epilogue.  The function carries no call and no absolute
; data reference, so its code is position-independent.
;
; The Watcom register calling convention passes the first argument in eax, the
; second in edx, and the third in ebx: the destination, the source, and the
; byte count.
.386
.model flat
public memmove_
.code
memmove_ proc near
        push ecx
        push esi
        push edi
        db 89h, 0C7h                    ; mov edi, eax, the 89h form the original compiler emits
        db 89h, 0D6h                    ; mov esi, edx, the 89h form the original compiler emits
        db 89h, 0D9h                    ; mov ecx, ebx, the 89h form the original compiler emits
        push es
        mov eax, ds
        mov es, eax
        push edi
        db 89h, 0C8h                    ; mov eax, ecx, the 89h form the original compiler emits
        shr ecx, 2
        db 0F2h, 0A5h                   ; repne movsd, the form the original compiler emits
        mov cl, al
        and cl, 3
        db 0F2h, 0A4h                   ; repne movsb, the form the original compiler emits
        pop edi
        pop es
        db 89h, 0F8h                    ; mov eax, edi, the 89h form the original compiler emits
        pop edi
        pop esi
        pop ecx
        ret
memmove_ endp
end
