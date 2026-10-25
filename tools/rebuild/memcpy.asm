; The rebuilt `memcpy` module.
;
; The original sits at linear 0x8c150 in STARTH.EXE and holds 14 bytes and 8
; instructions: `pushad`, the register moves, `shr ecx,2`, `rep movsd`, `popad`, and
; `ret`.  The function carries no call and no data reference, so its code is
; position-independent and matches the original byte for byte.
;
; The Watcom register calling convention passes the first argument in eax, the second
; in edx, and the third in ebx, so the module copies `ebx` bytes, counted in whole
; dwords, from the source in edx to the destination in eax.
.386
.model flat
public memcpy_
.code
memcpy_ proc near
        pushad
        mov esi, edx
        mov edi, eax
        mov ecx, ebx
        shr ecx, 2
        rep movsd
        popad
        ret
memcpy_ endp
end
