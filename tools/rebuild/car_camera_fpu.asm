; The FPU run-time stubs of the `car_camera` module.
;
; The module converts the nearest distance and the move length to an integer.
; The original calls the Watcom helper at 0x4d9f4, which sets the truncate
; rounding mode and runs `frndint`.  The shared `fpu_stub.asm` defines `__CHP`
; as a bare return, which leaves the default round-to-nearest mode.  This stub
; keeps the `__8087` and `__init_387_emulator` no-ops and the `_fltused_`
; marker, and it makes `__CHP` truncate the value on the FPU stack.
;
; The file also holds the `sqrt`, `sin`, `cos`, and `asin` stubs.  This harness
; links no math library, so the module takes these functions from the hardware.
; The original uses the routines at 0x22bec, 0x4da72, 0x4da66, and 0xc5e42.
.386
.model flat
public __8087
public __init_387_emulator
public __CHP
public sqrt_
public sin_
public cos_
public asin_
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
        fld qword ptr [ebp + 8]
        fsqrt
        pop ebp
        ret 8
sqrt_ endp
sin_ proc near
        push ebp
        mov ebp, esp
        fld qword ptr [ebp + 8]
        fsin
        pop ebp
        ret 8
sin_ endp
cos_ proc near
        push ebp
        mov ebp, esp
        fld qword ptr [ebp + 8]
        fcos
        pop ebp
        ret 8
cos_ endp
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
