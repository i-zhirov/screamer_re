; The rebuilt `cd_audio_tick` module, hand-written to reproduce the original
; body.
;
; The original sits at linear 0x25f90 in STARTH.EXE and holds 38 instructions
; and the full 132-byte extent: `pushal`, the tick counter, the early tick path,
; the disc-info path, the state and Q-channel paths, and the `popal; ret`
; epilogue.
;
; The C form reorders the branches and uses a different prologue.  This source
; holds the original body exactly.  The counter, state, and gate addresses and
; the four call targets are link-time operands, which the whole-image
; relocation rewrites.
.386
.model flat
public cd_audio_tick_
extern _DAT_0016000d : dword
extern _DAT_00166688 : dword
extern _DAT_0016669a : dword
extern cd_audio_disc_info_request_ : near
extern cd_audio_q_channel_read_ : near
extern cd_audio_table_build_ : near
extern sound_driver_set_ : near
.code
cd_audio_tick_ proc near
        db 60h
        inc dword ptr [_DAT_0016000d]
        mov eax, dword ptr [_DAT_0016000d]
        cmp eax, 4
        jl less
        ; `xor eax,eax` in the store encoding (`33 c0`) that the original uses.
        db 33h, 0c0h
        mov dword ptr [_DAT_0016000d], eax
        call cd_audio_disc_info_request_
        cmp al, 0ffh
        jne disc
        mov eax, 1
        mov dword ptr [_DAT_00166688], eax
        db 33h, 0c0h
gate:
        cmp eax, dword ptr [_DAT_0016669a]
        je done
        call sound_driver_set_
        db 61h
        ret
disc:
        bt dword ptr [_DAT_00166688], 0
        jae done
        call cd_audio_q_channel_read_
        ; `or al,al` in the store encoding (`0a c0`) that the original uses.
        db 0ah, 0c0h
        jne done
        db 33h, 0c0h
        mov dword ptr [_DAT_00166688], eax
        call cd_audio_table_build_
        ; `mov eax,0` in the store encoding (`b8 00 00 00 00`).
        db 0b8h, 0, 0, 0, 0
        jmp gate
less:
        call cd_audio_q_channel_read_
        cmp al, 2
        je setflag
        cmp al, 0fh
        je setflag
        jmp done
setflag:
        mov eax, 1
        mov dword ptr [_DAT_00166688], eax
done:
        db 61h
        ret
cd_audio_tick_ endp
end
