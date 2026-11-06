; The FPU run-time stubs of the `player_car_contact_update` module.
;
; The module uses the arc tangent.  The original calls the Watcom helper at
; 0xc2254.  This harness links no math library, so the stub here takes the
; value from the hardware FPU.
;
; The module converts no value to an integer, so `__CHP` stays a bare return.
; The stubs keep the `__8087` and `__init_387_emulator` no-ops and the
; `_fltused_` marker.
.386
.model flat
public __8087
public __init_387_emulator
public __CHP
public atan_
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
atan_ proc near
        push ebp
        mov ebp, esp
        fld qword ptr [ebp + 8]
        fld1
        fpatan
        pop ebp
        ret 8
atan_ endp
.data
public _fltused_
_fltused_ dd 0
end
