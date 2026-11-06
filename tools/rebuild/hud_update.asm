; The rebuilt `hud_update` module, hand-written to reproduce the original body.
;
; The original sits at linear 0x27a34 in STARTH.EXE and holds 11 instructions
; and 38 bytes: the frame `push ebp` and `mov ebp,esp`, the guard
; `cmp dword ptr [DAT_0011ce44], 3` with the skip of the first two overlay
; calls, the guard `test byte ptr [DAT_0011ce44], 2` with the skip of the third
; overlay call, and the epilogue `pop ebp` and `ret`.
;
; The C build with `-of+` adds one `mov esp,ebp` in the epilogue, so this source
; holds the original instruction shape exactly.  The only differing operands are
; the data displacement and the five direct branch displacements, which the
; oracle normalizes as link-time addresses.
.386
.model flat
public hud_update_
extern _DAT_0011ce44 : dword
extern hud_overlay_0_ : near
extern hud_overlay_1_ : near
extern hud_overlay_2_ : near
.code
hud_update_ proc near
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        cmp dword ptr [_DAT_0011ce44], 3
        je short skip_calls
        call hud_overlay_0_
        call hud_overlay_1_
skip_calls:
        test byte ptr [_DAT_0011ce44], 2
        je short done
        call hud_overlay_2_
done:
        pop ebp
        ret
hud_update_ endp
end
