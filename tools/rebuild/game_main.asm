; The rebuilt `main` module as minimal assembly.
;
; The original at 0x10020 is called by `main_invoker_` with `eax`/`edx`.  The
; 10.0a C compiler emits `push ebp` before the register saves and cannot hold
; the race result in the `[ebp-4]` slot with an `ebx` test, so the function is
; written in assembly to reproduce the original form.  It is the only module
; forced to assembly for this reason.
.486
.model flat
public game_main_
extrn memory_alloc_:near
extrn list_create_:near
extrn memory_available_:near
extrn printf_:near
extrn engine_init_:near
extrn race_loop_:near
extrn bios_video_:near
extrn error_report_:near
extrn exit_proc_:near
extrn frame_update_:near
extrn engine_shutdown_:near
extrn list_destroy_:near
extrn release_:near
extrn _DAT_000062ac:dword
extrn _DAT_00007ed8:dword
.code
game_main_ proc near
        push ebx
        push edx
        push ebp
        mov ebp, esp
        sub esp, 4
        mov eax, 0x4c
        call memory_alloc_
        test eax, eax
        je alloc_done
        mov edx, 0x96
        call list_create_
alloc_done:
        mov dword ptr [_DAT_000062ac], eax
        call memory_available_
        cmp eax, 0x61a800
        jge normal
        push 4
        call printf_
        add esp, 4
        mov eax, dword ptr [_DAT_000062ac]
        mov edx, 0x61a800
        call memory_available_
        sub edx, eax
        push edx
        push 0x29
        call printf_
        mov eax, dword ptr [_DAT_000062ac]
        add esp, 8
        jmp cleanup
normal:
        call engine_init_
loop_top:
        cmp dword ptr [_DAT_00007ed8], 1
        je loop_done
        call race_loop_
        mov dword ptr [ebp-4], eax
        mov ebx, dword ptr [ebp-4]
        test ebx, ebx
        je update
        mov eax, 3
        call bios_video_
        mov eax, dword ptr [ebp-4]
        call error_report_
        mov eax, dword ptr [ebp-4]
        call exit_proc_
update:
        call frame_update_
        jmp loop_top
loop_done:
        call engine_shutdown_
cleanup:
        mov eax, dword ptr [_DAT_000062ac]
        test eax, eax
        je done
        xor edx, edx
        call list_destroy_
        call release_
done:
        mov esp, ebp
        pop ebp
        pop edx
        pop ebx
        ret
game_main_ endp
end
