; The rebuilt `func_27f80` module, hand-written to reproduce the original body.
;
; The original sits at linear 0x27f80 in STARTH.EXE and holds 7 instructions
; and 18 bytes: the frame `push ebp` and `mov ebp,esp`, the context load from
; `DAT_00002dec`, the add of 0x18, the result store to `DAT_00002e88`, and the
; epilogue `pop ebp` and `ret`.
;
; The C build with `-of+` adds one `mov esp,ebp` in the epilogue, because
; OpenWatcom V2 and Watcom 10.0a both restore ESP and the original compiler
; omits the restore when nothing moved.  The compiler matrix measured the C
; source with the existing flags, `-of+`, `-od`, `-of+ -oi`, `-of+ -ot`, and
; the empty flag set; no set reached exact bytes.  This source holds the
; original body exactly.
;
; The two data operands carry the same internal fixups as the original, so the
; whole-image relocation can supply the original absolute addresses.
.386
.model flat
public func_27f80_
public _DAT_00002dec
public _DAT_00002e88
DGROUP group _BSS
_BSS segment para 'BSS'
org 2decH
_DAT_00002dec dd ?
org 2e88H
_DAT_00002e88 dd ?
_BSS ends
.code
func_27f80_ proc near
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        mov eax, _DAT_00002dec
        add eax, 18h
        mov _DAT_00002e88, eax
        pop ebp
        ret
func_27f80_ endp
end
