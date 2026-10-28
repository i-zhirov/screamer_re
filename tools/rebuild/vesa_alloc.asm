; The `dpmi_alloc_dos` stub of the `vesa_set_mode` harness.
;
; The module calls the DOS memory allocator and uses the returned block in
; `edi`.  The stub returns the harness info block in `edi` and 0 in `eax`, so
; the module runs the VBE mode info call against the block.
.386
.model flat
public dpmi_alloc_dos_
extrn _info_block:byte
.code
dpmi_alloc_dos_ proc near
        lea edi, _info_block
        xor eax, eax
        ret
dpmi_alloc_dos_ endp
end
