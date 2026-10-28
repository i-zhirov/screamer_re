; The rebuilt `memset` module (the run-time library version).
;
; The original sits at linear 0xd1ec0 in STARTH.EXE and holds 12 instructions
; and 24 bytes: the saves of `eax` and `ecx`, the value byte replicated over
; the whole dword, the count moved to `ecx`, the call to the run-time core at
; `0xe0860`, and the epilogue.  The function carries no absolute data
; reference, so the only difference from the original is the call displacement.
;
; The Watcom register calling convention passes the first argument in eax, the
; second in edx, and the third in ebx: the destination, the value, and the byte
; count.  The core takes the destination in eax, the replicated value in edx,
; and the count in ecx.
;
; The module is named `memset_runtime`, because the game-side `memset` module at
; `tools/rebuild/memset.asm` already carries the name `memset`.
.386
.model flat
public memset_runtime_
extrn memset_core_:near
.code
memset_runtime_ proc near
        push eax
        push ecx
        db 88h, 0D6h                    ; mov dh, dl, the 88h form the original compiler emits
        shl edx, 8
        db 88h, 0F2h                    ; mov dl, dh, the 88h form the original compiler emits
        shl edx, 8
        db 88h, 0F2h                    ; mov dl, dh, the 88h form the original compiler emits
        db 89h, 0D9h                    ; mov ecx, ebx, the 89h form the original compiler emits
        call memset_core_
        pop ecx
        pop eax
        ret
memset_runtime_ endp
end
