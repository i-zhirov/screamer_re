; The FPU run-time stub of the `player_car_spin_update` module.
;
; The module converts the counter and the sign to an integer.  The original
; uses the Watcom helper at 0x4d9f4, which truncates toward zero.  This stub
; keeps the FPU control no-ops, the float-used marker, and a `__CHP` that
; truncates toward zero.  The module uses no math routine.
.386
.model flat
public __8087
public __init_387_emulator
public __CHP
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
.data
public _fltused_
_fltused_ dd 0
end
