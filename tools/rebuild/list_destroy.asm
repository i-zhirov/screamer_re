; The rebuilt `list_destroy` module, hand-written to reproduce the original
; local slot order.
;
; The original sits at linear 0x2ed77 in STARTH.EXE and holds 25 instructions
; and 54 bytes.  It saves ebx, ecx, esi, edi, and ebp, builds the frame, and
; uses three local slots: the first parameter at [ebp-8], the second parameter
; at [ebp-4], and the return temporary at [ebp-0xc].
;
; The C compilers (9.01d, 9.5, 10.0, and 10.0a) place the compiler-generated
; return temporary in the highest slot and the parameters below it.  This
; source pins the slots in the original order.  The only link-time value is the
; `free` call target, which the oracle normalizes.
.386
.model flat
public list_destroy_
extern free_ : near
.code
list_destroy_ proc near
        push ebx
        push ecx
        push esi
        push edi
        push ebp
        ; `mov ebp, esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        ; `sub esp, 0ch` with the long immediate (`81 ec 0c 00 00 00`).
        db 81h, 0ech, 0ch, 00h, 00h, 00h
        mov [ebp-8], eax
        mov [ebp-4], edx
        mov eax, [ebp-8]
        cmp dword ptr [eax], 0
        je short done
        mov eax, [ebp-8]
        mov eax, [eax]
        call free_
done:
        mov eax, [ebp-8]
        mov [ebp-0ch], eax
        mov eax, [ebp-0ch]
        ; `mov esp, ebp` in the store encoding (`89 ec`).
        db 89h, 0ech
        pop ebp
        pop edi
        pop esi
        pop ecx
        pop ebx
        ret
list_destroy_ endp
end
