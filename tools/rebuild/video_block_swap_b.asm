; The rebuilt `video_block_swap_b` module.
;
; The original sits at linear 0x107a1 in STARTH.EXE and holds 18 instructions
; and 52 bytes: `pushad`, the video buffer pointer loaded from `DAT_00121528`,
; the far pointer at `0x3e580` bytes ahead, the two counters, the dword swap
; loop, and the epilogue.
;
; The module swaps 200 rows of 160 dwords between the front of the video buffer
; and the region `0xf960` dwords ahead.  The front walks forward and the far
; pointer walks backward by one row per pass.
;
; The module is assembly, because the original is library-style code: it holds
; `pushad`/`popad` and no EBP frame.
.386
.model flat
public video_block_swap_b_
extrn _DAT_00121528:dword
.code
video_block_swap_b_ proc near
        pushad
        mov esi, dword ptr [_DAT_00121528]
        mov edi, esi
        add edi, 0x3e580
        mov edx, 0xc8
row_loop:
        mov ecx, 0xa0
col_loop:
        mov eax, dword ptr [esi]
        mov ebx, dword ptr [edi]
        mov dword ptr [edi], eax
        mov dword ptr [esi], ebx
        add esi, 4
        add edi, 4
        loop col_loop
        sub edi, 0x500
        dec edx
        jne row_loop
        popad
        ret
video_block_swap_b_ endp
end
