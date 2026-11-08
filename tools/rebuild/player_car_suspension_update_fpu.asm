; The FPU run-time stubs of the `player_car_suspension_update` module.
;
; The module converts the wheel travel to an integer.  The original calls the
; Watcom helper at 0x4d9f4, which sets the truncate rounding mode and runs
; `frndint`.  The shared `fpu_stub.asm` defines `__CHP` as a bare return, which
; leaves the default round-to-nearest mode.  This stub keeps the `__8087` and
; `__init_387_emulator` no-ops and the `_fltused_` marker, and it makes `__CHP`
; truncate the value on the FPU stack.
;
; The file also holds the `cos`, `atan`, and `fabs` stubs.  This harness links
; no math library.  The original uses the routines at 0x4da66 and 0xc2254.
.386
.model flat
public __8087
public __init_387_emulator
public __CHP
public cos_
public atan_
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
cos_ proc near
        push ebp
        mov ebp, esp
        fld qword ptr [ebp + 8]
        fcos
        pop ebp
        ret 8
cos_ endp
atan_ proc near
        push ebp
        mov ebp, esp
        fld qword ptr [ebp + 8]
        fld1
        fpatan
        pop ebp
        ret 8
atan_ endp
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
