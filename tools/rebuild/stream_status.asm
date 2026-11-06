; The rebuilt `stream_status` module, hand-written to reproduce the original
; body.
;
; The original sits at linear 0x4c7e0 in STARTH.EXE and holds 21 instructions
; and 38 bytes: the six register saves, the frame `push ebp` and `mov ebp,esp`,
; the local area `sub esp,8`, the parameter slot, the word read at
; `stream + 0x70`, the result slot, and the epilogue `mov esp,ebp`, the six
; register restores, and `ret`.
;
; The parameter arrives in EAX.  The function returns the status word at
; `stream + 0x70`.
;
; The C build with `-of+ -od` holds a different prologue order, different local
; offsets, and a `lea` epilogue, so this source holds the original instruction
; shape exactly.  The only differing values are the local slots, which are not
; link-time addresses.
.386
.model flat
public stream_status_
.code
stream_status_ proc near
        push ebx
        push ecx
        push edx
        push esi
        push edi
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        ; `sub esp,8` with the 32-bit immediate (`81 ec 08 00 00 00`) that the
        ; original uses, not the assembler's shorter sign-extended form.
        db 81h, 0ech, 08h, 00h, 00h, 00h
        mov [ebp-4], eax
        mov eax, [ebp-4]
        mov eax, dword ptr [eax+70h]
        mov [ebp-8], eax
        mov eax, [ebp-8]
        ; `mov esp,ebp` in the store encoding (`89 ec`).
        db 89h, 0ech
        pop ebp
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        ret
stream_status_ endp
end
