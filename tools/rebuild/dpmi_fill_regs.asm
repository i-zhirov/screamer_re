; The rebuilt `dpmi_fill_regs` module.
;
; The original sits at linear 0x4bf23 in STARTH.EXE and holds 16 instructions
; and 40 bytes: the `edx` save, the pointer moved from `eax` to `edx`, the six
; segment reads `cs`, `ds`, `es`, `ss`, `fs`, and `gs`, the six word stores,
; and the epilogue.  The function carries no call and no absolute data
; reference, so its code is position-independent and matches the original byte
; for byte.
;
; The function fills the 12-byte real-mode segment set in the order `es`, `cs`,
; `ss`, `ds`, `fs`, and `gs`, at offsets 0, 2, 4, 6, 8, and 10.  The pointer
; arrives in `eax`, the first argument of the register calling convention.
.386
.model flat
public dpmi_fill_regs_
.code
dpmi_fill_regs_ proc near
        push edx
        db 89h, 0C2h                    ; mov edx, eax, the 89h form the original compiler emits
        mov eax, cs
        mov word ptr [edx + 2], ax
        mov eax, ds
        mov word ptr [edx + 6], ax
        mov eax, es
        mov word ptr [edx], ax
        mov eax, ss
        mov word ptr [edx + 4], ax
        mov eax, fs
        mov word ptr [edx + 8], ax
        mov eax, gs
        mov word ptr [edx + 0xa], ax
        pop edx
        ret
dpmi_fill_regs_ endp
end
