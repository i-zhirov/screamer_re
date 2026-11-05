; The rebuilt `memset` module.
;
; The original sits at linear 0x8c160 in STARTH.EXE and holds 12 instructions and
; 25 bytes: `pushal`, the register moves, the value replication, `shr ecx,2`,
; `rep stosd`, `popal`, and `ret`.  The function carries no call and no data
; reference, so its code is position-independent and matches the original byte
; for byte.
;
; The Watcom register calling convention passes the first argument in eax, the
; second in edx, and the third in ebx.  The call sites load the destination into
; eax, the byte count into edx, and the fill value into ebx, and the function
; fills `count >> 2` dwords with the value byte replicated.
.386
.model flat
public memset_
.code
memset_ proc near
        pushad
        mov edi, eax
        mov eax, ebx
        mov ecx, edx
        mov ah, al
        mov bx, ax
        shl eax, 0x10
        mov ax, bx
        shr ecx, 2
        rep stosd
        popad
        ret
memset_ endp
end
