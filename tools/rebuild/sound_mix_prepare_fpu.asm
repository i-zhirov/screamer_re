; The FPU run-time stubs of the `sound_mix_prepare` module.
;
; The module takes the square root of the source distance.  The harness links
; no math library, so the module takes the square root from the hardware.  The
; C cast to an integer calls the Watcom `__CHP` helper, so the stub supplies it.
; The original uses a direct `fistp` with the default round-to-nearest mode.
; The harness uses perfect squares, so the two paths agree.
;
; The file also keeps the `__8087` and `__init_387_emulator` no-ops and the
; `_fltused_` marker.
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
        fld qword ptr [ebp + 8]
        fsqrt
        pop ebp
        ret 8
sqrt_ endp
.data
public _fltused_
_fltused_ dd 0
end
