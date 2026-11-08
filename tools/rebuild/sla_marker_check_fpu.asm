; The FPU run-time stubs of the `sla_marker_check` module.
;
; The module takes the square root of the summed squares and compares it with
; the marker radius as a float.  It converts no value to an integer, so `__CHP`
; stays a bare return.  The original calls the square root at 0x22bec.  This
; harness links no math library, so the file supplies the square root from the
; hardware.  The file also supplies the FPU control init, the 387 emulator
; init, and the float-used marker.
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
