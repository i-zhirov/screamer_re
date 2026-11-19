; The rebuilt `memory_alloc_small` module, hand-written to reproduce the
; original body.
;
; The original sits at linear 0x2f172 in STARTH.EXE and holds 49 instructions
; and 147 bytes: the four register saves, the frame and 0x3c local area, the
; DPMI 0x0501 block, the two DPMI calls, the status branch, the address and
; size reassembly, and the epilogue.
;
; The C form with `-od` numbers the locals in the opposite order.  This source
; holds the original body exactly.  The two call targets are link-time
; operands, which the whole-image relocation rewrites.
.386
.model flat
public memory_alloc_small_
extern dpmi_fill_regs_ : near
extern dpmi_call_ : near
.code
memory_alloc_small_ proc near
        push ecx
        push esi
        push edi
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        ; `sub esp,0x3c` in the store encoding (`81 ec`) that the original uses.
        db 81h, 0ech, 3ch, 0, 0, 0
        mov dword ptr [ebp - 0ch], eax
        mov dword ptr [ebp - 8], edx
        mov dword ptr [ebp - 4], ebx
        mov dword ptr [ebp - 2ch], 501h
        mov eax, dword ptr [ebp - 8]
        ; `sar eax,0x10` in the store encoding (`c1 f8`) that the original uses.
        db 0c1h, 0f8h, 10h
        mov dword ptr [ebp - 28h], eax
        mov eax, dword ptr [ebp - 8]
        and eax, 0ffffh
        mov dword ptr [ebp - 24h], eax
        lea eax, [ebp - 38h]
        call dpmi_fill_regs_
        lea ecx, [ebp - 38h]
        lea ebx, [ebp - 2ch]
        lea edx, [ebp - 2ch]
        mov eax, 31h
        call dpmi_call_
        cmp dword ptr [ebp - 14h], 0
        je success
        mov dword ptr [ebp - 3ch], 0
        jmp done
success:
        mov ebx, dword ptr [ebp - 1ch]
        shl ebx, 10h
        mov edx, dword ptr [ebp - 18h]
        and edx, 0ffffh
        mov eax, dword ptr [ebp - 4]
        or ebx, edx
        mov dword ptr [eax], ebx
        mov edx, dword ptr [ebp - 28h]
        shl edx, 10h
        mov eax, dword ptr [ebp - 24h]
        and eax, 0ffffh
        or edx, eax
        mov dword ptr [ebp - 10h], edx
        mov eax, dword ptr [ebp - 10h]
        mov dword ptr [ebp - 3ch], eax
done:
        mov eax, dword ptr [ebp - 3ch]
        ; `mov esp,ebp` in the store encoding (`89 ec`) that the original uses.
        db 89h, 0ech
        pop ebp
        pop edi
        pop esi
        pop ecx
        ret
memory_alloc_small_ endp
end
