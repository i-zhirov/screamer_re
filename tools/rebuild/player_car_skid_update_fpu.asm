; The FPU and math stubs of the `player_car_skid_update` module.
;
; The module forms the shake angle with the cosine, the two shake terms with
; the sine and the cosine, and the effect value with the square root.  The
; original calls the sine at 0x4da72, the cosine at 0x4da66, and the
; square-root wrapper at 0x22bec.
;
; This harness links no math library.  The stub keeps the `__8087` and
; `__init_387_emulator` no-ops and the `_fltused_` marker, and it takes `sin`,
; `cos`, and `sqrt` from the hardware.  It also makes `__CHP` truncate toward
; zero, because a float-to-integer conversion uses the Watcom helper at
; 0x4d9f4 in the original.
.386
.model flat
public __8087
public __init_387_emulator
public __CHP
public sqrt_
public sin_
public cos_
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
.data
public _fltused_
_fltused_ dd 0
end
