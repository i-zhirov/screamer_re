; The FPU run-time stubs of the `race_state_update` harness.
;
; The float code of the module references the FPU run-time symbols `__8087` and
; `__init_387_emulator`.  The compiler emits these names without the trailing
; underscore, so a C stub cannot provide them; the stubs here stand in for the
; FPU control init and the 387 emulator init.
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
end
