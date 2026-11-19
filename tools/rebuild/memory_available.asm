; The rebuilt `memory_available` module, hand-written to reproduce the original
; body.
;
; The original sits at linear 0x2f8ca in STARTH.EXE and holds 27 instructions
; and 58 bytes: the six register saves, the frame and 0xc local area, the two
; probe calls, the sum, the temporary, and the epilogue.
;
; The C form with `-of+` keeps the same frame and locals but numbers them in
; the opposite order.  This source holds the original body exactly.  The two
; probe call targets are link-time operands, which the whole-image relocation
; rewrites.
.386
.model flat
public memory_available_
extern probe_free_ : near
extern probe_size_ : near
.code
memory_available_ proc near
        push ebx
        push ecx
        push edx
        push esi
        push edi
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        ; `sub esp,0xc` in the store encoding (`81 ec`) that the original uses.
        db 81h, 0ech, 0ch, 0, 0, 0
        mov dword ptr [ebp - 4], eax
        mov eax, dword ptr [ebp - 4]
        call probe_free_
        ; `mov edx,eax` in the store encoding (`89 c2`) that the original uses.
        db 89h, 0c2h
        mov eax, dword ptr [ebp - 4]
        call probe_size_
        ; `add edx,eax` in the store encoding (`01 c2`) that the original uses.
        db 01h, 0c2h
        mov dword ptr [ebp - 8], edx
        mov eax, dword ptr [ebp - 8]
        mov dword ptr [ebp - 0ch], eax
        mov eax, dword ptr [ebp - 0ch]
        ; `mov esp,ebp` in the store encoding (`89 ec`) that the original uses.
        db 89h, 0ech
        pop ebp
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        ret
memory_available_ endp
end
