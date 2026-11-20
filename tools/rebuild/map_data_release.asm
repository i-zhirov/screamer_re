; The rebuilt `map_data_release` module, hand-written to reproduce the original
; body.
;
; The original sits at linear 0x4cef4 in STARTH.EXE and holds 15 instructions
; and 53 bytes: the saved EDX, the frame, the two resource names, the two free
; calls, the map grid key and find call, and the epilogue.
;
; The C form with `-of+` adds `mov esp,ebp` and reorders the argument setup.
; This source holds the original body exactly.  The three call targets, the
; archive head, the map grid key, and the two resource names are link-time
; addresses, which the whole-image relocation rewrites.
.386
.model flat
public map_data_release_
extern _map_obj_list : byte
extern _map_unr_file : byte
extern _DAT_0021574 : dword
extern _DAT_0021468 : dword
extern resource_free_name_ : near
extern resource_find_ : near
.code
map_data_release_ proc near
        push edx
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        mov edx, offset _map_obj_list
        mov eax, _DAT_0021574
        call resource_free_name_
        mov edx, offset _map_unr_file
        mov eax, _DAT_0021574
        call resource_free_name_
        mov edx, _DAT_0021468
        mov eax, _DAT_0021574
        call resource_find_
        pop ebp
        pop edx
        ret
map_data_release_ endp
end
