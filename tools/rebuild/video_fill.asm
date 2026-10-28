; The alternative assembly build of the `video_fill` module.
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
; This file is the alternative build, not the active module source.  The C build
; at `video_fill.c` is the active module; it holds the original shape with one
; extra `lea esp,[ebp-8]`, because OpenWatcom V2 saves the registers after the
; frame and always emits an ESP restore, while the original compiler saves them
; before the frame and omits the restore.  This assembly build holds the
; original shape exactly and passes the identity check, with only the two
; link-time addresses differing.  A build with the original Watcom 10.x
; compiler should reproduce the C code shape, and then the C build becomes the
; exact match.
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
