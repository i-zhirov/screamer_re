; The rebuilt `dpmi_int31_entry` module.
;
; The original sits at linear 0xd4410 in STARTH.EXE and holds 28 instructions
; and 56 bytes: the saves, the dispatcher call at `0xd4448`, the register block
; writes, the flag write, the pops, the segment writes, and the epilogue.
;
; The dispatcher at `0xd4448` turns the interrupt number into the `int N; ret`
; stub address from the table at `cs:0xc44ee`, loads the real-mode registers
; from the block, and runs the stub through the real-mode interrupt path.  The
; module writes the returned registers, the flags, and the segment values back
; into the block.
;
; The module is assembly, because the original is a run-time routine with the
; segment and flag juggling.  The rebuild calls the dispatcher through the
; external stub `dpmi_dispatch_`, so the only difference from the original is
; the call displacement.
.386
.model flat
public dpmi_int31_entry_
extrn dpmi_dispatch_:near
.code
dpmi_int31_entry_ proc near
        push ebp
        push es
        push ebx
        push ds
        push edx
        call dpmi_dispatch_
        push ds
        push edi
        mov ebp, esp
        mov edi, dword ptr [ebp + 8]
        mov ds, word ptr [ebp + 0xc]
        mov dword ptr [edi], eax
        mov dword ptr [edi + 4], ebx
        mov dword ptr [edi + 8], ecx
        mov dword ptr [edi + 0xc], edx
        mov dword ptr [edi + 0x10], esi
        pop dword ptr [edi + 0x14]
        sbb eax, eax
        mov dword ptr [edi + 0x18], eax
        pop eax
        pop ebx
        pop ebx
        pop ebx
        mov word ptr [ebx + 6], ax
        mov word ptr [ebx], es
        pop es
        pop ebp
        ret
dpmi_int31_entry_ endp
end
