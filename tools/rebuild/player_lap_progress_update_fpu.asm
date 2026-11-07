; The FPU run-time stub of the `player_lap_progress_update` module.
;
; The module converts the lap index and the node index to floats and adds the
; segment ratio.  It converts no float to an integer, so this stub keeps the
; default round-to-nearest `__CHP` and supplies only the FPU control no-ops and
; the float-used marker.
.386
.model flat
public __8087
public __init_387_emulator
public __CHP
.code
__8087 proc near
        ret
__8087 endp
__init_387_emulator proc near
        ret
__init_387_emulator endp
__CHP proc near
        ret
__CHP endp
.data
public _fltused_
_fltused_ dd 0
end
