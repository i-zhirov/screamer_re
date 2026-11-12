; The FPU and math stubs of the `sqrt` module.
;
; The function guards the negative argument and then calls the square-root
; core at 0xc6cbe.  The core of the original tests the sign bit of the
; argument and returns the hardware square root for a non-negative value.
;
; This harness links no math library, so the stub supplies `sqrt_core_` from
; the hardware.  The module converts no value to an integer, so `__CHP` stays a
; bare return.  The stub keeps the `__8087` and `__init_387_emulator` no-ops
; and the `_fltused_` marker.
.386
.model flat
public __8087
public __init_387_emulator
public __CHP
public sqrt_core_
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
sqrt_core_ proc near
        push ebp
        mov ebp, esp
        fld qword ptr [ebp + 8]
        fsqrt
        pop ebp
        ret 8
sqrt_core_ endp
.data
public _fltused_
_fltused_ dd 0
end
