; The FPU and math stubs of the `direction_angle` module.
;
; The function takes the absolute value of the inverse sine of the x argument.
; The original calls the Watcom inverse-sine helper at 0xc5e42.  The harness
; links no math library and does not load the constant segment, so this stub
; supplies `asin_` from the hardware.
;
; The stub forms sqrt(1 - x * x) and calls `fpatan` with x and the root.  The
; result is the inverse sine for an argument in the range -1 to 1.  The stub
; keeps the `__8087` and `__init_387_emulator` no-ops, the `__CHP` no-op, and
; the `_fltused_` marker.
.386
.model flat
public __8087
public __init_387_emulator
public __CHP
public asin_
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
asin_ proc near
        push ebp
        mov ebp, esp
        fld qword ptr [ebp + 8]
        fld st(0)
        fmul st(0), st(0)
        fld1
        fsubrp st(1), st(0)
        fsqrt
        fpatan
        pop ebp
        ret 8
asin_ endp
.data
public _fltused_
_fltused_ dd 0
end
