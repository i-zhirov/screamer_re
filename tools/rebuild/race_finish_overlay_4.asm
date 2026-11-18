; The rebuilt `race_finish_overlay_4` module, hand-written to reproduce the
; original body.
;
; The original sits at linear 0x240a0 in STARTH.EXE and holds 15 instructions
; and 33 bytes: the saved EBX, ECX, EDX, and EBP, the colour and line values,
; the runtime text pointer, the font zero, the panel draw call, and the
; epilogue.
;
; The C build with `-of+` adds a frame restore and reorders the argument setup,
; because OpenWatcom V2 restores ESP and keeps the zero with another register.
; This source holds the original body exactly.  The text pointer and the call
; target are link-time addresses, which the whole-image relocation rewrites.
.386
.model flat
public race_finish_overlay_4_
extern _DAT_00118ff0 : dword
extern panel_draw_centered_ : near
.code
race_finish_overlay_4_ proc near
        push ebx
        push ecx
        push edx
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        mov ecx, 2
        mov edx, 5ah
        mov eax, _DAT_00118ff0
        ; `xor ebx,ebx` in the store encoding (`31 db`) that the original uses.
        db 31h, 0dbh
        call panel_draw_centered_
        pop ebp
        pop edx
        pop ecx
        pop ebx
        ret
race_finish_overlay_4_ endp
end
