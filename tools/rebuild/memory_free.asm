; The rebuilt `memory_free` module, hand-written to reproduce the original
; body.
;
; The original sits at linear 0x2f205 in STARTH.EXE and holds 33 instructions
; and 90 bytes: the five register saves, the frame and 0x34 local area, the
; argument slots, the DPMI block and segment set, the two calls, and the
; epilogue.
;
; The C form with `-of+` numbers the locals in the opposite order and emits
; `shr` instead of the original `sar`.  This source holds the original body
; exactly.  The two call targets are link-time operands, which the whole-image
; relocation rewrites.
.386
.model flat
public memory_free_
extern dpmi_fill_regs_ : near
extern dpmi_call_ : near
.code
memory_free_ proc near
        push ebx
        push ecx
        push esi
        push edi
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        ; `sub esp,0x34` in the store encoding (`81 ec`) that the original uses.
        db 81h, 0ech, 34h, 0, 0, 0
        mov dword ptr [ebp - 8], eax
        mov dword ptr [ebp - 4], edx
        mov dword ptr [ebp - 28h], 502h
        mov eax, dword ptr [ebp - 4]
        ; `sar eax,0x10` in the store encoding (`c1 f8`) that the original uses.
        db 0c1h, 0f8h, 10h
        mov dword ptr [ebp - 18h], eax
        mov eax, dword ptr [ebp - 4]
        and eax, 0ffffh
        mov dword ptr [ebp - 14h], eax
        lea eax, [ebp - 34h]
        call dpmi_fill_regs_
        lea ecx, [ebp - 34h]
        lea ebx, [ebp - 28h]
        lea edx, [ebp - 28h]
        mov eax, 31h
        call dpmi_call_
        mov eax, dword ptr [ebp - 10h]
        mov dword ptr [ebp - 0ch], eax
        mov eax, dword ptr [ebp - 0ch]
        ; `mov esp,ebp` in the store encoding (`89 ec`) that the original uses.
        db 89h, 0ech
        pop ebp
        pop edi
        pop esi
        pop ecx
        pop ebx
        ret
memory_free_ endp
end
