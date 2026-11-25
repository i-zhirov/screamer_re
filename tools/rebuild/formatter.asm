; The stream sink of the rebuilt `formatter` module.
;
; The original `__fprtf` sink at 0xd428e calls the Watcom `fputc` at 0xde2db.
; The rebuilt `__fprtf` keeps the `FILE` record, but the stream buffer helpers
; are compatibility stubs, so this sink writes the byte through the DOS handle
; at `FILE + 0x00`.  The output is unbuffered, which is enough for the rebuilt
; `printf` paths and keeps the stream path independent of the buffer state.
;
; The C caller passes the `FILE` in eax and the character in edx, and it
; expects eax, ecx, and edx to change.
.386
.model flat
public formatter_stream_sink_
.code
formatter_stream_sink_ proc near
        push ebx
        push ecx
        push edx
        mov ebx, dword ptr [eax]
        push edx
        mov edx, esp
        mov ecx, 1
        mov eax, 0x4000
        int 0x21
        add esp, 4
        pop edx
        pop ecx
        pop ebx
        ret
formatter_stream_sink_ endp
end
