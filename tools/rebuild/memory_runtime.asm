; The interim heap/memory run-time helpers.
;
; `memset_core_` is the run-time byte fill.  The register convention passes the
; destination in eax, the replicated value in edx, and the count in ecx, from
; `memset_runtime.asm`.  `stack_grow_0_` and `stack_grow_1_` are the two stack
; helpers that `main_invoker.asm` calls.  The globals are defined by
; `memory_runtime.c`.
;
; The module is a compatibility layer; it resolves the symbols in the
; whole-image link and does not claim a byte-faithful recovery.
.386
.model flat
public memset_core_
public stack_grow_0_
public stack_grow_1_
extrn _DAT_00004290:dword
extrn _DAT_000061c0:dword
.code
memset_core_ proc near
        push edi
        mov edi, eax
        mov eax, edx
        cld
        rep stosb
        mov eax, edi
        pop edi
        ret
memset_core_ endp
stack_grow_0_ proc near
        mov eax, esp
        sub eax, dword ptr [_DAT_00004290]
        ret
stack_grow_0_ endp
stack_grow_1_ proc near
        mov dword ptr [_DAT_000061c0], 8000h
        ret
stack_grow_1_ endp
end
