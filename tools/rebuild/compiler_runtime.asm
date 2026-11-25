; The raw compiler and run-time labels of the compiler cluster.
;
; CLIB3R references `__int7` and `__int7_pl3` from the FPU emulator.  The
; defining member is `emu387.asm` in EMU387.LIB, which the whole-image link
; does not include.  The two stubs return from the interrupt with `iret`, so
; an unexpected coprocessor fault cannot leave the stack unbalanced.
;
; The V2 C startup also pulls the DLL startup member `dllmncwd.c`, which
; references `__DLLstart_`.  The game is an EXE, so the DLL entry is not
; called; the stub reports success in EAX.
;
; The labels are raw, because their linker names do not follow the Watcom C
; naming rules that `compiler_runtime.c` uses.
.386
.model flat
public __int7
public __int7_pl3
public __DLLstart_
.code
__int7 proc near
        iret
__int7 endp
__int7_pl3 proc near
        iret
__int7_pl3 endp
__DLLstart_ proc near
        mov eax, 1
        ret
__DLLstart_ endp
end
