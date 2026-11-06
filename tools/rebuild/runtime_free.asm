; The rebuilt run-time `free` module.
;
; The original sits at linear 0xd2008 in STARTH.EXE and holds 35 instructions
; and 94 bytes: the four register saves, the free-list walk, the heap release
; call at `0xe0a84`, the largest-free-block update, the heap flag clear, and the
; epilogue.
;
; The function walks the free list at `DAT_001031d0` from the head pointer.  It
; stops when the released block lies before the next free block, releases the
; block through the heap routine, and keeps the largest free size in
; `DAT_001031d8`.  The module is assembly, because the original is the run-time
; allocator code and the rebuild keeps the exact instruction sequence.
.386
.model flat
public free_
extrn _free_list:dword
extrn _free_list_end:dword
extrn _largest_free:dword
extrn _heap_flag:byte
extrn heap_free_:near
.code
free_ proc near
        push ebx
        push ecx
        push edx
        push ebp
        mov ebx, eax
        mov ecx, dword ptr [_free_list]
        jmp check
body:
        cmp ecx, ebx
        ja next
        cmp ebx, edx
        jb found
next:
        mov ecx, dword ptr [ecx + 8]
check:
        mov edx, dword ptr [ecx + 8]
        test edx, edx
        jne body
found:
        mov eax, offset _free_list
        mov edx, ds
        and edx, 0xffff
        mov eax, ebx
        mov ebx, ecx
        call heap_free_
        cmp ecx, dword ptr [_free_list_end]
        je done
        mov ebp, dword ptr [_largest_free]
        mov edx, dword ptr [ecx + 0x14]
        cmp edx, ebp
        jbe done
        mov dword ptr [_largest_free], edx
done:
        xor ah, ah
        mov byte ptr [_heap_flag], ah
        pop ebp
        pop edx
        pop ecx
        pop ebx
        ret
free_ endp
end
