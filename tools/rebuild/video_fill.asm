; The rebuilt `video_fill` module.
;
; The original sits at linear 0x2aa98 in STARTH.EXE and holds 12 instructions and
; 30 bytes: the register saves `ebx` and `edx`, the frame `push ebp` and
; `mov ebp,esp`, the size pair `edx=0x190` (400) and `eax=0x1e0` (480), the
; buffer pointer loaded from `DAT_00121528` into `ebx`, the call to the video
; copy at `0x8beb8`, and the epilogue `pop ebp`, `pop edx`, `pop ebx`, and `ret`.
;
; The original C source is `video_copy(480, 400, DAT_00121528)`.  The pointer
; `DAT_00121528` holds the video buffer address: `video_buf_load` at `0x4c8f0`
; stores the loaded `VIDEO_BUF` resource pointer there.
;
; The module is assembly, because the OpenWatcom V2 C compiler cannot reproduce
; the original code shape.  The original compiler saves the registers before the
; frame and omits the ESP restore when the frame holds no local variable, while
; OpenWatcom V2 saves after the frame and always emits `lea esp,[ebp-8]` in the
; epilogue.  The extra instruction breaks the instruction-sequence match, so the
; C rebuild cannot pass the identity check; the assembly rebuild holds the
; original shape and differs only in the two link-time addresses.
.386
.model flat
public video_fill_
extrn _DAT_00121528:dword
extrn video_copy_:near
.code
video_fill_ proc near
        push ebx
        push edx
        push ebp
        db 89h, 0E5h                    ; mov ebp, esp, the 89h form the original compiler emits
        mov edx, 0x190
        mov eax, 0x1e0
        mov ebx, dword ptr [_DAT_00121528]
        call video_copy_
        pop ebp
        pop edx
        pop ebx
        ret
video_fill_ endp
end
