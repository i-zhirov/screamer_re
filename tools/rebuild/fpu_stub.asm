; The FPU run-time stubs of the float harnesses.
;
; The float code of the modules references the FPU run-time symbols `__8087`
; and `__init_387_emulator`, and the linker needs the float-used marker
; `_fltused_`.  The compiler emits these names without the trailing underscore,
; so a C stub cannot provide them; the stubs here stand in for the FPU control
; init, the 387 emulator init, and the float-used marker.
.386
.model flat
public __8087
public __init_387_emulator
.code
__8087 proc near
        ret
__8087 endp
__init_387_emulator proc near
        ret
__init_387_emulator endp
.data
public _fltused_
_fltused_ dd 0
end
