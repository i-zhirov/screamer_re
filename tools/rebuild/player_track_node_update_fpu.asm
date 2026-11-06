; The FPU run-time stubs of the `player_track_node_update` module.
;
; The module converts the radius distance, the segment length, and the object
; distance to an integer.  The original calls the Watcom helper at 0x4d9f4,
; which sets the truncate rounding mode and runs `frndint`.  The shared
; `fpu_stub.asm` defines `__CHP` as a bare return, which leaves the default
; round-to-nearest mode, so a value rounds away from zero.  This stub keeps the
; `__8087` and `__init_387_emulator` no-ops and the `_fltused_` marker, and it
; makes `__CHP` truncate the value on the FPU stack.
;
; The file also holds the `sqrt` stub.  A C sqrt needs the literals 0.5 and
; 1.0, and the linker of this harness reads them at a raw low address, so the
; C body produces NaN.  The hardware `fsqrt` needs no literal.
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
