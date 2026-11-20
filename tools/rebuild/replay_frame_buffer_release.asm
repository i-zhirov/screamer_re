; The rebuilt `replay_frame_buffer_release` module, hand-written to reproduce
; the original body.
;
; The original sits at linear 0x22d44 in STARTH.EXE and holds 11 instructions
; and 30 bytes: the saved EDX, the frame, the `REPLAY` name argument, the
; archive context load, the resource free call, the buffer clear, and the
; epilogue.
;
; The C build with `-of+` adds one `mov esp,ebp` and a frame restore, because
; OpenWatcom V2 restores ESP and the original compiler omits the restore when
; nothing moved.  This source holds the original body exactly.  The three data
; operands and the call target are link-time addresses, which the whole-image
; relocation rewrites.
.386
.model flat
public replay_frame_buffer_release_
extern _replay_name : byte
extern _DAT_000062ac : dword
extern _DAT_0018e5c : dword
extern resource_free_name_ : near
.code
replay_frame_buffer_release_ proc near
        push edx
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        mov edx, offset _replay_name
        mov eax, _DAT_000062ac
        call resource_free_name_
        ; `xor edx,edx` in the store encoding (`31 d2`) that the original uses.
        db 31h, 0d2h
        mov _DAT_0018e5c, edx
        pop ebp
        pop edx
        ret
replay_frame_buffer_release_ endp
end
