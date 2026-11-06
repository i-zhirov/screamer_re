; The rebuilt `music_bank_reset` module, hand-written to reproduce the original
; body.
;
; The original sits at linear 0x2f9b0 in STARTH.EXE and holds 14 instructions
; and 46 bytes: the frame `push ebp` and `mov ebp,esp`, the local slot
; `sub esp,4`, the parameter store, the four reloads of the parameter, the
; three field stores, and the epilogue `mov esp,ebp`, `pop ebp`, and `ret`.
;
; The parameter arrives in EAX.  The function clears the fields `+0x14` and
; `+0x10` and sets the field `+0x04` to `0xffffffff` in the music bank record.
;
; The C build with `-of+` adds a different frame shape, so this source holds
; the original instruction shape exactly.  The only differing values are the
; local slot and the field offsets, which are not link-time addresses.
.386
.model flat
public music_bank_reset_
.code
music_bank_reset_ proc near
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        sub esp, 4
        mov [ebp-4], eax
        mov eax, [ebp-4]
        mov dword ptr [eax+14h], 0
        mov eax, [ebp-4]
        mov dword ptr [eax+10h], 0
        mov eax, [ebp-4]
        mov dword ptr [eax+4], 0ffffffffh
        mov eax, [ebp-4]
        ; `mov esp,ebp` in the store encoding (`89 ec`).
        db 89h, 0ech
        pop ebp
        ret
music_bank_reset_ endp
end
