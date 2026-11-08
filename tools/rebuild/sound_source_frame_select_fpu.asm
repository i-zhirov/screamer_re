; The FPU run-time stubs of the `sound_source_frame_select` module.
;
; The original converts the square root to an integer with a raw `fistp`, so
; the FPU control word keeps the default round-to-nearest mode.  The shared
; `fpu_stub.asm` leaves `__CHP` as a no-op, and this stub keeps the same
; no-op.  The file also holds the `sqrt` stub, because this harness links no
; math library.  The original takes the square root from the hardware.
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
        fld qword ptr [ebp + 8]
        fsqrt
        pop ebp
        ret 8
sqrt_ endp
.data
public _fltused_
_fltused_ dd 0
end
