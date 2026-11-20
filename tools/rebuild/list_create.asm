; The rebuilt `list_create` module, hand-written to reproduce the original
; body.
;
; The original sits at linear 0x2ecf8 in STARTH.EXE and holds 46 instructions
; and 127 bytes: the five register saves, the frame and 0xc local area, the
; pool setup, the record allocation, the pointer check, the two store paths,
; the zero fill, and the epilogue.
;
; The C form with `-od` numbers the locals in the opposite order.  This source
; holds the original body exactly.  The three call targets are link-time
; operands, which the whole-image relocation rewrites.
.386
.model flat
public list_create_
extern pool_init_ : near
extern malloc_rt_ : near
extern memset_runtime_ : near
.code
list_create_ proc near
        push ebx
        push ecx
        push esi
        push edi
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        ; `sub esp,0xc` in the store encoding (`81 ec`) that the original uses.
        db 81h, 0ech, 0ch, 0, 0, 0
        mov dword ptr [ebp - 8], eax
        mov dword ptr [ebp - 4], edx
        mov eax, dword ptr [ebp - 8]
        call pool_init_
        ; `imul eax,[ebp-4],0x1c` in the original encoding.
        db 6bh, 45h, 0fch, 1ch
        call malloc_rt_
        mov edx, eax
        mov eax, dword ptr [ebp - 8]
        mov dword ptr [eax], edx
        mov eax, dword ptr [ebp - 8]
        cmp dword ptr [eax], 0
        jne allocated
        mov eax, dword ptr [ebp - 8]
        mov dword ptr [eax + 4], 0
        mov eax, dword ptr [ebp - 8]
        mov edx, dword ptr [eax + 4]
        mov eax, dword ptr [ebp - 8]
        mov dword ptr [eax + 8], edx
        jmp done
allocated:
        mov eax, dword ptr [ebp - 4]
        mov edx, dword ptr [ebp - 8]
        mov dword ptr [edx + 8], eax
        mov eax, dword ptr [ebp - 8]
        mov dword ptr [eax + 4], 0
        ; `imul ebx,[ebp-4],0x1c` in the original encoding.
        db 6bh, 5dh, 0fch, 1ch
        xor edx, edx
        mov eax, dword ptr [ebp - 8]
        mov eax, dword ptr [eax]
        call memset_runtime_
done:
        mov eax, dword ptr [ebp - 8]
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
list_create_ endp
end
