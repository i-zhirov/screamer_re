; The rebuilt `frame_step` module, hand-written to reproduce the original body.
;
; The original sits at linear 0x2aa84 in STARTH.EXE and holds 7 instructions
; and 20 bytes: the frame, the three calls to `default_data_copy` at 0x10193,
; `joystick_update` at 0x24da0, and `frame_timer` at 0x1fc38, and the epilogue.
;
; The C build with `-of+` adds one `mov esp,ebp`, because OpenWatcom V2 always
; restores ESP and the original compiler omits the restore.  This source holds
; the original body exactly.  The only differing values are the call targets,
; which the oracle normalizes as link-time addresses.
.386
.model flat
public frame_step_
extern default_data_copy_ : near
extern joystick_update_ : near
extern frame_timer_ : near
.code
frame_step_ proc near
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        call default_data_copy_
        call joystick_update_
        call frame_timer_
        pop ebp
        ret
frame_step_ endp
end
