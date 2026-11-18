; The rebuilt `screen_setup_cleanup` module, hand-written to reproduce the
; original body.
;
; The original sits at linear 0x15150 in STARTH.EXE and holds 12 instructions
; and 42 bytes: the frame `push ebp` and `mov ebp,esp`, the text mode call, the
; keyboard hook with the value 0, the five release and shutdown calls, and the
; epilogue `pop ebp` and `ret`.
;
; The C build with `-of+` adds one `mov esp,ebp` in the epilogue, because
; OpenWatcom V2 restores ESP and the original compiler omits the restore when
; nothing moved.  The restored ESP makes the harness frame differ.  This source
; holds the original body exactly.  The seven call targets are link-time
; operands, which the whole-image relocation rewrites.
.386
.model flat
public screen_setup_cleanup_
extern video_text_mode_ : near
extern keyboard_hook_ : near
extern materials_release_ : near
extern fonts_release_ : near
extern sprites_release_ : near
extern sound_system_shutdown_ : near
extern dos_datetime_set_ : near
.code
screen_setup_cleanup_ proc near
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        call video_text_mode_
        ; `xor eax,eax` in the store encoding (`31 c0`) that the original uses.
        db 31h, 0c0h
        call keyboard_hook_
        call materials_release_
        call fonts_release_
        call sprites_release_
        call sound_system_shutdown_
        call dos_datetime_set_
        pop ebp
        ret
screen_setup_cleanup_ endp
end
