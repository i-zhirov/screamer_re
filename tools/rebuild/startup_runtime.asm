; The interim startup/run-time port and termination helpers.
;
; `inportb_` and `outportb_` implement the port I/O that `keyboard_hook.c`
; calls.  The register convention passes the port in eax and the value in edx
; for `outportb`.  `exit_terminate_` is the DOS process terminator that
; `exit.c`, `exit_proc`, and `startup_terminate` call.
;
; The module is a compatibility layer; it resolves the symbols in the
; whole-image link and does not claim a byte-faithful recovery.
.386
.model flat
public inportb_
public outportb_
public exit_terminate_
.code
inportb_ proc near
        mov edx, eax
        in al, dx
        movzx eax, al
        ret
inportb_ endp
outportb_ proc near
        push ebx
        mov ebx, eax
        mov eax, edx
        mov dx, bx
        out dx, al
        pop ebx
        ret
outportb_ endp
exit_terminate_ proc near
        mov ah, 4ch
        int 21h
        ret
exit_terminate_ endp
end
