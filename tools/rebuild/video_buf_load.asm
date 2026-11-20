; The rebuilt `video_buf_load` module, hand-written to reproduce the original
; body.
;
; The original sits at linear 0x4c8f0 in STARTH.EXE and holds 18 instructions
; and 53 bytes: the saved EBX and EDX, the frame, the name and key arguments,
; the stream load, the resource lookup, the two result stores, the null guard,
; the release call, and the epilogue.
;
; The C form with `-of+` adds `mov esp,ebp`, reorders the argument setup, and
; reloads the stream after the call.  This source holds the original body
; exactly.  The three data operands and the two call targets are link-time
; addresses, which the whole-image relocation rewrites.
.386
.model flat
public video_buf_load_
extern _DAT_00021574 : dword
extern _DAT_00021504 : dword
extern _DAT_00121528 : dword
extern _video_key : byte
extern resource_get_ : near
extern stream_release_ : near
.code
video_buf_load_ proc near
        push ebx
        push edx
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        mov ebx, 4b500h
        mov edx, offset _video_key
        mov eax, _DAT_00021574
        call resource_get_
        mov _DAT_00021504, eax
        mov _DAT_00121528, eax
        test eax, eax
        jne done
        mov eax, _DAT_00021574
        call stream_release_
done:
        pop ebp
        pop edx
        pop ebx
        ret
video_buf_load_ endp
end
