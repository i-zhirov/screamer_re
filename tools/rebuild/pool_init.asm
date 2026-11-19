; The rebuilt `pool_init` module, hand-written to reproduce the original body.
;
; The original sits at linear 0x2f904 in STARTH.EXE and holds 44 instructions
; and 129 bytes: the six register saves, the frame and 0x30 local area, the
; DPMI 0x0500 block, the two DPMI calls, the status branch, the two page
; conversions, and the epilogue.
;
; The C form with `-od` numbers the locals in the opposite order.  This source
; holds the original body exactly.  The two call targets are link-time
; operands, which the whole-image relocation rewrites.
.386
.model flat
public pool_init_
extern dpmi_fill_regs_ : near
extern dpmi_call_ : near
.code
pool_init_ proc near
        push ebx
        push ecx
        push edx
        push esi
        push edi
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        ; `sub esp,0x30` in the store encoding (`81 ec`) that the original uses.
        db 81h, 0ech, 30h, 0, 0, 0
        mov dword ptr [ebp - 4], eax
        mov dword ptr [ebp - 24h], 500h
        mov eax, dword ptr [ebp - 4]
        add eax, 1ch
        mov dword ptr [ebp - 10h], eax
        lea eax, [ebp - 30h]
        call dpmi_fill_regs_
        lea ecx, [ebp - 30h]
        lea ebx, [ebp - 24h]
        lea edx, [ebp - 24h]
        mov eax, 31h
        call dpmi_call_
        cmp dword ptr [ebp - 0ch], 0
        je success
        mov dword ptr [ebp - 8], 1
        jmp done
success:
        mov eax, dword ptr [ebp - 4]
        shl dword ptr [eax + 24h], 0ch
        mov eax, dword ptr [ebp - 4]
        cmp dword ptr [eax + 3ch], 0
        jle zero
        mov eax, dword ptr [ebp - 4]
        shl dword ptr [eax + 3ch], 0ch
        jmp setdone
zero:
        mov eax, dword ptr [ebp - 4]
        mov dword ptr [eax + 3ch], 0
setdone:
        mov dword ptr [ebp - 8], 0
done:
        mov eax, dword ptr [ebp - 8]
        ; `mov esp,ebp` in the store encoding (`89 ec`) that the original uses.
        db 89h, 0ech
        pop ebp
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        ret
pool_init_ endp
end
