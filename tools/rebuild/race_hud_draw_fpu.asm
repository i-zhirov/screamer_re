; The FPU run-time stub of the `race_hud_draw` module.
;
; The module converts the race clock, the lap clock, and the lap value to an
; integer.  The original calls the Watcom helper at 0x4d9f4, which sets the
; truncate rounding mode and runs `frndint`.  The shared `fpu_stub.asm` leaves
; round-to-nearest, so this stub makes `__CHP` truncate the value on the FPU
; stack.  The stub keeps the `__8087` and `__init_387_emulator` no-ops and the
; `_fltused_` marker.
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
