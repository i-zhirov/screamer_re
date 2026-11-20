; The rebuilt `replay_playback_begin` module, hand-written to reproduce the
; original body.
;
; The original sits at linear 0x22db8 in STARTH.EXE and holds 19 instructions
; and 64 bytes: the saved EBX, ESI, and EBP, the frame buffer gate, the replay
; read mode, the cleared index, the two shared view cells, and the playback bit.
;
; The C build with `-of+` saves the frame first, uses other registers for the
; zero and the one, and reverses the store order.  This source holds the
; original body exactly.  The six data operands are link-time addresses, which
; the whole-image relocation rewrites.
.386
.model flat
public replay_playback_begin_
extern _DAT_00018e60 : dword
extern _DAT_00018e1c : byte
extern _DAT_00018e6c : dword
extern _DAT_00018e64 : dword
extern _DAT_00001df0 : dword
extern _DAT_0010a9c : dword
.code
replay_playback_begin_ proc near
        push ebx
        push esi
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        cmp _DAT_00018e60, 0
        je done
        mov ebx, 1
        mov ah, _DAT_00018e1c
        ; `xor esi,esi` in the store encoding (`31 f6`) that the original uses.
        db 31h, 0f6h
        mov _DAT_00018e6c, ebx
        mov _DAT_00018e64, esi
        mov _DAT_00001df0, ebx
        or ah, 40h
        mov _DAT_0010a9c, ebx
        mov _DAT_00018e1c, ah
done:
        pop ebp
        pop esi
        pop ebx
        ret
replay_playback_begin_ endp
end
