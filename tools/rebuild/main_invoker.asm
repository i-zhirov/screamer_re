; The rebuilt `main invoker` module.
;
; The original sits at linear 0xd4324 in STARTH.EXE and holds 26 instructions
; and 79 bytes: the frame, the stack alignment, the two stack grow calls, the
; main call at `0x10020` with the argument pair, the exit call at `0x300be`, and
; the epilogue.
;
; The module is the Watcom startup invoker: it aligns the stack, grows the
; stack, calls the game main, and calls the exit function.
;
; The module is assembly, because the original adjusts the stack pointer
; directly, which the C compiler does not express.
.486
.model flat
public main_invoker_
extrn _DAT_00004298:dword
extrn _DAT_0000429c:dword
extrn _DAT_00027e24:dword
extrn _DAT_00027e28:dword
extrn stack_grow_0_:near
extrn stack_grow_1_:near
extrn game_main_:near
extrn exit_proc_:near
.code
main_invoker_ proc near
        push edx
        push ebp
        mov ebp, esp
        mov edx, dword ptr [_DAT_00004298]
        add edx, 3
        and dl, 0xfc
        call stack_grow_0_
        cmp edx, eax
        jae no_adjust
        mov eax, edx
        sub esp, eax
        mov eax, esp
        jmp adjust_done
no_adjust:
        xor eax, eax
adjust_done:
        mov edx, dword ptr [_DAT_00004298]
        add eax, edx
        mov dword ptr [_DAT_0000429c], eax
        call stack_grow_1_
        mov edx, dword ptr [_DAT_00027e28]
        mov eax, dword ptr [_DAT_00027e24]
        call game_main_
        call exit_proc_
        mov esp, ebp
        pop ebp
        pop edx
        ret
main_invoker_ endp
end
