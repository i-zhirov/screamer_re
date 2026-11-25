; The rebuilt `text_measure` module.
;
; The original sits at linear 0x254a8 in STARTH.EXE and holds 104 instructions
; and 360 bytes up to the `ret` at 0x2560f.  The next function starts at
; 0x25610, so the store record keeps the 376-byte extent and sets no separate
; function size.  The identity check uses 104 instructions.
;
; The original is a self-contained Watcom 10.0a routine.  It saves all the
; registers with `pushad`, copies the four arguments into BSS cells, scans the
; glyph row for the widest advance, then walks the text and writes the width
; and height through the two output pointers.  The C build keeps the same state
; in locals, so the V2 compiler cannot reproduce the register and BSS sequence.
; The transliteration keeps the original cells and the original instruction
; order.
;
; The four arguments arrive in the Watcom register order: the width pointer in
; eax, the height pointer in edx, the text pointer in ebx, and the font index
; in ecx.  The font index selects a 12-byte record from DAT_00015438 and a
; 0x500-byte glyph row from DAT_00015434.  The record holds the height at
; +0x00, the first character at +0x04, and the count at +0x08.  Each glyph
; holds its advance in the first 32-bit slot of 8 bytes.
;
; The BSS cells are the original module state.  The four font pointers and the
; count stay external, because the harness supplies them.
.386
.model flat
public text_measure_
extrn _DAT_00015434:dword
extrn _DAT_00015438:dword
extrn _DAT_00015440:dword
.code
text_measure_ proc near
        pushad
        mov dword ptr [tm_15610], eax
        mov dword ptr [tm_15614], edx
        mov dword ptr [tm_15458], ebx
        mov dword ptr [tm_1545c], ecx
        cmp ecx, dword ptr [_DAT_00015440]
        jb short tm_measure
        mov eax, 2
        jmp tm_done
tm_measure:
        mov eax, 0xa0
        imul eax, eax, 8
        imul eax, dword ptr [tm_1545c]
        mov dword ptr [tm_15464], eax
        mov eax, dword ptr [tm_1545c]
        imul eax, eax, 0xc
        mov esi, dword ptr [_DAT_00015438]
        add esi, eax
        mov eax, dword ptr [esi]
        mov edi, dword ptr [tm_15614]
        mov dword ptr [edi], eax
        mov eax, dword ptr [esi + 4]
        mov dword ptr [tm_15470], eax
        mov eax, dword ptr [esi + 8]
        mov dword ptr [tm_15474], eax
        mov esi, dword ptr [tm_15458]
        mov edi, dword ptr [tm_15610]
        pushad
        mov ecx, dword ptr [tm_15474]
        mov edi, dword ptr [_DAT_00015434]
        add edi, dword ptr [tm_15464]
        xor eax, eax
        jmp short tm_scan
        db 2Eh, 8Bh, 0C0h
        db 2Eh, 8Bh, 0C0h
        db 2Eh, 8Bh, 0C0h
        db 2Eh, 8Bh, 0C0h
        nop
tm_scan:
        mov ebx, dword ptr [edi]
        cmp eax, ebx
        jg short tm_wider
        mov eax, ebx
tm_wider:
        add edi, 8
        dec ecx
        jne short tm_scan
        add eax, 2
        mov dword ptr [tm_15490], eax
        mov eax, 1
        mov dword ptr [tm_15488], eax
        mov dword ptr [tm_1548c], eax
        popad
        xor eax, eax
        mov dword ptr [edi], eax
        jmp short tm_loop
        db 2Eh, 8Bh, 0C0h
        nop
tm_loop:
        xor eax, eax
        mov al, byte ptr [esi]
        or al, al
        je tm_done
        mov ebx, dword ptr [tm_15470]
        cmp eax, ebx
        jl short tm_control
        add ebx, dword ptr [tm_15474]
        cmp eax, ebx
        jge short tm_control
        sub eax, dword ptr [tm_15470]
        mov esi, dword ptr [_DAT_00015434]
        imul eax, eax, 8
        add esi, eax
        add esi, dword ptr [tm_15464]
        test dword ptr [tm_15488], 0xf
        jne short tm_glyph
        mov ebx, dword ptr [tm_15490]
        jmp short tm_add
tm_glyph:
        mov ebx, dword ptr [esi]
        test dword ptr [tm_1548c], 0xf
        je short tm_widen_skip
        inc ebx
tm_widen_skip:
        inc ebx
tm_add:
        add dword ptr [edi], ebx
tm_control:
        cmp al, 1
        jne short tm_control2
        xor eax, eax
        mov dword ptr [tm_15488], eax
        jmp short tm_next
tm_control2:
        cmp al, 2
        jne short tm_control3
        mov dword ptr [tm_15488], eax
        jmp short tm_next
tm_control3:
        cmp al, 3
        jne short tm_control4
        xor eax, eax
        mov dword ptr [tm_1548c], eax
        jmp short tm_next
tm_control4:
        cmp al, 4
        jne short tm_next
        mov dword ptr [tm_1548c], eax
tm_next:
        mov esi, dword ptr [tm_15458]
        inc esi
        mov dword ptr [tm_15458], esi
        jmp tm_loop
tm_done:
        cld
        popad
        ret
text_measure_ endp

.data
tm_15458 dd 0
tm_1545c dd 0
tm_15464 dd 0
tm_15470 dd 0
tm_15474 dd 0
tm_15488 dd 0
tm_1548c dd 0
tm_15490 dd 0
tm_15610 dd 0
tm_15614 dd 0
end
