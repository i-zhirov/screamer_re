; The FPU and math stub of the `angle_filter` module.
;
; The module forms the small-offset move with the sine.  The original calls the
; sine at 0x4da72.  This harness links no math library, so the stub keeps the
; `__8087` and `__init_387_emulator` no-ops and the `_fltused_` marker, and it
; takes `sin` from the hardware.  It also makes `__CHP` truncate toward zero,
; as the other float harnesses do.
.386
.model flat
public __8087
public __init_387_emulator
public __CHP
public fabs_
public sin_
.code
__8087 proc near
        ret
__8087 endp
__init_387_emulator proc near
        ret
__init_387_emulator endp
__CHP proc near
        push eax
        wait
        fnstcw word ptr [esp]
        wait
        push dword ptr [esp]
        mov byte ptr [esp + 1], 0x1f
        fldcw word ptr [esp]
        frndint
        fldcw word ptr [esp + 4]
        wait
        lea esp, [esp + 8]
        ret
__CHP endp
fabs_ proc near
        push ebp
        mov ebp, esp
        fld qword ptr [ebp + 8]
        fabs
        pop ebp
        ret 8
fabs_ endp
sin_ proc near
        push ebp
        mov ebp, esp
        fld qword ptr [ebp + 8]
        fsin
        pop ebp
        ret 8
sin_ endp
.data
public _fltused_
_fltused_ dd 0
end
