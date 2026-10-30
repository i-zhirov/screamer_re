; The rebuilt `video_block_swap_a` module.
;
; The original sits at linear 0x1075f in STARTH.EXE and holds 25 instructions
; and 66 bytes: `pushad`, the video buffer pointer loaded from `DAT_00121528`,
; the far pointer at `0x27c` bytes ahead, the two counters, the byte-swap and
; dword swap loop, and the epilogue.
;
; The module byte-swaps and swaps dwords between the front of the video buffer
; and the region `0x9f` dwords ahead: 80 passes of 400 dwords, and the far
; pointer walks backward by one dword per pass.
;
; The module is assembly, because the original is library-style code: it holds
; `pushad`/`popad` and no EBP frame.
.486
.model flat
public video_block_swap_a_
extrn _DAT_00121528:dword
.code
video_block_swap_a_ proc near
        pushad
        mov esi, dword ptr [_DAT_00121528]
        mov edi, esi
        add edi, 0x27c
        mov edx, 0x50
row_setup:
        mov ecx, 0x190
        push esi
        push edi
col_loop:
        mov eax, dword ptr [esi]
        mov ebx, dword ptr [edi]
        bswap eax
        bswap ebx
        mov dword ptr [edi], eax
        mov dword ptr [esi], ebx
        add esi, 0x280
        add edi, 0x280
        loop col_loop
        pop edi
        pop esi
        add esi, 4
        sub edi, 4
        dec edx
        jne row_setup
        popad
        ret
video_block_swap_a_ endp
end
