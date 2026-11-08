; The FPU run-time stubs of the `opponent_path_update` module.
;
; The module takes the arc sine, the square root, and the absolute value, and
; it converts the target values to an integer.  The original calls the Watcom
; helpers at 0xc5e42, 0x22bec, and 0x4d9f4, and it runs `fabs` on the FPU.  This
; harness links no math library, so the stubs here use the hardware FPU.
;
; The original truncates toward zero through the helper at 0x4d9f4.  The shared
; `fpu_stub.asm` defines `__CHP` as a bare return, which leaves the default
; round-to-nearest mode, so a negative target can round away from zero.  This
; stub makes `__CHP` truncate the value on the FPU stack.  The stubs keep the
; `__8087` and `__init_387_emulator` no-ops and the `_fltused_` marker.
.386
.model flat
public __8087
public __init_387_emulator
public __CHP
public sqrt_
public asin_
public fabs_
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
fabs_ proc near
        push ebp
        mov ebp, esp
        fld qword ptr [ebp + 8]
        fabs
        pop ebp
        ret 8
fabs_ endp
.data
public _fltused_
_fltused_ dd 0
end
