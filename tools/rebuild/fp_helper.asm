; The rebuilt `fp_helper` module.
;
; The original sits at linear 0x4d9f4 in STARTH.EXE and holds 12 instructions
; and 29 bytes: the `eax` save, the control word saved to the stack, the
; rounding control set to truncate, `frndint`, the control word restored, and
; the stack adjusted.  The function carries no call and no absolute data
; reference, so its code is position-independent.
;
; The function is the Watcom floating-point helper that rounds the value on the
; FPU stack to an integer.  It runs between float operations that need the
; integer result, and it takes no argument and returns no value.
.386
.model flat
public fp_helper_
.code
fp_helper_ proc near
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
fp_helper_ endp
end
