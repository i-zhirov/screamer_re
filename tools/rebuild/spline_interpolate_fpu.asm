; The FPU and math stubs of the `spline_interpolate` module.
;
; The module measures the three chords with the square root and converts the
; tangents and the result to integers.  The original calls the square-root
; wrapper at 0x22bec, which returns 0.0 for a negative argument and the square
; root otherwise, and the Watcom helper at 0x4d9f4, which sets the truncate
; rounding mode and runs `frndint`.
;
; This harness links no math library.  The shared `fpu_stub.asm` defines
; `__CHP` as a bare return, which leaves the default round-to-nearest mode, and
; it supplies no `sqrt`.  This stub keeps the `__8087` and
; `__init_387_emulator` no-ops and the `_fltused_` marker, makes `__CHP`
; truncate the value on the FPU stack, and takes `sqrt` from the hardware.  The
; negative-argument test follows the wrapper at 0x22bec, because the integer
; chord squares can overflow to a negative value.
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
