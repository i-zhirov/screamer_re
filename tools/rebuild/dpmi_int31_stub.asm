; The dispatcher stub of the `dpmi_int31_entry` harness.
;
; The module calls the dispatcher through this stub.  The stub sets the result
; registers to known values and returns.  The real-mode interrupt path of the
; original dispatcher cannot run in the flat harness, so the behavior run
; covers only the register return path.
.386
.model flat
public dpmi_dispatch_
.code
dpmi_dispatch_ proc near
        mov eax, 0x11111111
        mov ebx, 0x22222222
        mov ecx, 0x33333333
        mov edx, 0x44444444
        mov esi, 0x55555555
        ret
dpmi_dispatch_ endp
end
