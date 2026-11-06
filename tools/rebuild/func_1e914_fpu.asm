; The FPU and math stubs of the `func_1e914` module.
;
; The function normalizes the forward tangent with the square root.  The
; original calls the Watcom square-root helper at 0xc6cbe, which returns the
; hardware square root and routes a negative argument through the error path.
;
; This harness links no math library, so the stub supplies `sqrt_` from the
; hardware.  The negative-argument test returns 0.0, as the shared square-root
; wrapper at 0x22bec does.  The module converts no value to an integer, so
; `__CHP` stays a bare return.  The stub keeps the `__8087` and
; `__init_387_emulator` no-ops and the `_fltused_` marker.
.386
.model flat
public __8087
public __init_387_emulator
public __CHP
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
sqrt_ proc near
        push ebp
        mov ebp, esp
        fldz
        fcomp qword ptr [ebp + 8]
        fnstsw ax
        sahf
        jbe sqrt_positive
        fldz
        pop ebp
        ret 8
sqrt_positive:
        fld qword ptr [ebp + 8]
        fsqrt
        pop ebp
        ret 8
sqrt_ endp
.data
public _fltused_
_fltused_ dd 0
end
