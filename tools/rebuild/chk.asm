; The two symbols that the run-time free build needs.
; __CHK is the stack check helper the C compiler emits.  The helper returns at
; once, so the check is a no-op.
;
; _cstart_ is the start symbol that the compiler's main module references.  It runs a
; segment guard, sets ES to DS, and jumps to main_.
;
; The guard protects the flat-model assumption that the modules depend on.  The Watcom
; memcpy module in tools/rebuild/memcpy.asm copies through `rep movsd`, which reads
; through DS:ESI and writes through ES:EDI, and the print helper of every harness reads
; through DS:EDX.  A mismatch between the segments makes such a copy read correctly and
; write to the wrong place, which is a silent fault.  The guard writes a pattern to the
; stack through ES:EDI and reads it back through DS:ESI and through SS:ESP.  When a read
; differs, the harness spells SEGFAIL on the screen through the BIOS teletype, which needs
; no segment setup, and exits with the code 1, so the fault is loud.
.386
.model flat
public __CHK
public _cstart_
extrn main_:near
.code
__CHK proc near
        ret
__CHK endp
_cstart_ proc near
        pushfd
        pushad
        push    ds
        pop     es
        push    0
        mov     edi, esp
        mov     eax, 0A5A5A5A5h
        mov     dword ptr es:[edi], eax
        cmp     eax, dword ptr [edi]
        jne     guard_fail
        cmp     eax, dword ptr ss:[edi]
        jne     guard_fail
        add     esp, 4
        popad
        popfd
        jmp     main_
guard_fail:
        mov     ah, 0Eh
        mov     al, 'S'
        int     10h
        mov     al, 'E'
        int     10h
        mov     al, 'G'
        int     10h
        mov     al, 'F'
        int     10h
        mov     al, 'A'
        int     10h
        mov     al, 'I'
        int     10h
        mov     al, 'L'
        int     10h
        mov     al, 13
        int     10h
        mov     al, 10
        int     10h
        mov     ax, 4C01h
        int     21h
_cstart_ endp
end
