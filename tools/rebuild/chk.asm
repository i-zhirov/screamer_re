; The two symbols that the run-time free build needs.
; __CHK is the stack check helper the C compiler emits.  The helper returns at
; once, so the check is a no-op.
; _cstart_ is the start symbol that the compiler's main module references.  It sets
; ES to DS, because a string move writes through ES:EDI, and the Watcom memcpy
; module in tools/rebuild/memcpy.asm uses `rep movsd` for the copy.  The stub then
; jumps to main_, and the program exits through the DOS call itself.
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
        push    ds
        pop     es
        jmp     main_
_cstart_ endp
end
