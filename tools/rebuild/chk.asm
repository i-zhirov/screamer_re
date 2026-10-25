; The two symbols that the run-time free build needs.
; __CHK is the stack check helper the C compiler emits.  The helper returns at
; once, so the check is a no-op.
; _cstart_ is the start symbol that the compiler's main module references.  It
; jumps to main_, and the driver exits through the DOS call itself.
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
        jmp main_
_cstart_ endp
end
