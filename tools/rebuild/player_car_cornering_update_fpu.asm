; The FPU run-time stubs of the `player_car_cornering_update` module.
;
; The module uses the sine, the arc tangent, and the square root.  The original
; calls the Watcom helpers at 0x4da72, 0xc21e2, and 0x22bec.  This harness links
; no math library, so the stubs here take the values from the hardware FPU.
;
; The module converts no value to an integer, so `__CHP` stays a bare return.
; The stubs keep the `__8087` and `__init_387_emulator` no-ops and the
; `_fltused_` marker.
.386
.model flat
public __8087
public __init_387_emulator
public __CHP
public sin_
public atan2_
public sqrt_
.code
__8087 proc near
        ret
__8087 endp
__init_387_emulator proc near
        ret
__init_387_emulator endp
__CHP proc near
        ret
__CHP endp
sin_ proc near
        push ebp
        mov ebp, esp
        fld qword ptr [ebp + 8]
        fsin
        pop ebp
        ret 8
sin_ endp
atan2_ proc near
        push ebp
        mov ebp, esp
        fld qword ptr [ebp + 8]
        fld qword ptr [ebp + 16]
        fpatan
        pop ebp
        ret 16
atan2_ endp
sqrt_ proc near
        push ebp
        mov ebp, esp
        fld qword ptr [ebp + 8]
        fsqrt
        pop ebp
        ret 8
sqrt_ endp
.data
public _fltused_
_fltused_ dd 0
end
