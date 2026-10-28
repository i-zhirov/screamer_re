; The rebuilt `dpmi_call` module.
;
; The original sits at linear 0x4bf4b in STARTH.EXE and holds 14 instructions
; and 25 bytes: the saves of `esi`, `edi`, and `ebp`, the four arguments moved
; to the register set of the run-time entry, the call to the DPMI INT 31h entry
; at `0xd4410`, the result read from the third argument block, and the
; epilogue.  The function carries no absolute data reference, so the only
; difference from the original is the call displacement.
;
; The Watcom register calling convention passes the four arguments in eax, edx,
; ebx, and ecx: the interrupt number, the general register block, the result
; block, and the segment set.  The entry receives the interrupt number in esi,
; the general block in edi, the result block in edx, and the segment set in
; ebx.  The function returns the dword at offset 0 of the result block.
.386
.model flat
public dpmi_call_
extrn dpmi_int31_entry_:near
.code
dpmi_call_ proc near
        push esi
        push edi
        push ebp
        db 89h, 0DDh                    ; mov ebp, ebx, the 89h form the original compiler emits
        db 89h, 0CBh                    ; mov ebx, ecx, the 89h form the original compiler emits
        db 89h, 0C6h                    ; mov esi, eax, the 89h form the original compiler emits
        db 89h, 0D7h                    ; mov edi, edx, the 89h form the original compiler emits
        db 89h, 0EAh                    ; mov edx, ebp, the 89h form the original compiler emits
        call dpmi_int31_entry_
        mov eax, dword ptr [ebp]
        pop ebp
        pop edi
        pop esi
        ret
dpmi_call_ endp
end
