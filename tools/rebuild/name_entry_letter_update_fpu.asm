; The FPU and math stubs of the `name_entry_letter_update` module.
;
; The module converts the interpolated height, the progress completion, and the
; three angles to integers.  The original calls the Watcom helper at 0x4d9f4,
; which sets the truncate rounding mode and runs `frndint`.  This stub keeps the
; `__8087` and `__init_387_emulator` no-ops and the `_fltused_` marker, and it
; makes `__CHP` truncate the value on the FPU stack.
;
; The module calls the Watcom pow helper at 0xc21e2 only with the exponent 2.0.
; This harness links no math library, so the `pow_` stub squares the base and
; ignores the exponent.  That limit is safe because the module holds one pow
; call and its second argument is the literal 2.0.
.386
.model flat
public __8087
public __init_387_emulator
public __CHP
public pow_
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
pow_ proc near
        fld qword ptr [esp + 4]
        fmul st(0), st(0)
        ret 0x10
pow_ endp
.data
public _fltused_
_fltused_ dd 0
end
