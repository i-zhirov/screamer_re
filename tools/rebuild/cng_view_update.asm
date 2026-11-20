; The rebuilt `cng_view_update` module, hand-written to reproduce the original
; body.
;
; The original sits at linear 0x20360 in STARTH.EXE and holds 23 instructions
; and 98 bytes: the frame, the two gate tests, the seven player calls with the
; fixed player pointer 0xf91c, the argument-free player update, and the
; epilogue.
;
; The C form with `-of+` adds `mov esp,ebp` and reorders the calls.  This source
; holds the original body exactly.  The gate addresses and the eight call
; targets are link-time operands, which the whole-image relocation rewrites.
.386
.model flat
public cng_view_update_
extern _DAT_00118e1d : byte
extern _DAT_0011d260 : byte
extern opponent_score_update_ : near
extern player_car_view_update_ : near
extern player_physics_update_ : near
extern player_car_effect_update_ : near
extern player_step_update_ : near
extern player_opponent_update_ : near
extern player_update_ : near
extern player_car_link_ : near
public _player_object
DGROUP group _BSS
_BSS segment para 'BSS'
org 0f91cH
_player_object db ?
_BSS ends
.code
cng_view_update_ proc near
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        test byte ptr [_DAT_00118e1d], 1
        je done
        test byte ptr [_DAT_0011d260], 20h
        je done
        mov eax, offset _player_object
        call opponent_score_update_
        mov eax, offset _player_object
        call player_car_view_update_
        mov eax, offset _player_object
        call player_physics_update_
        mov eax, offset _player_object
        call player_car_effect_update_
        mov eax, offset _player_object
        call player_step_update_
        mov eax, offset _player_object
        call player_opponent_update_
        call player_update_
        mov eax, offset _player_object
        call player_car_link_
done:
        pop ebp
        ret
cng_view_update_ endp
end
