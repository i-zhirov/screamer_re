; The rebuilt `default_data_copy` module.
;
; The original sits at linear 0x10193 in STARTH.EXE and holds 16 instructions
; and 33 bytes: the register saves, the segment saves, the two block addresses,
; `cld`, the `rep movsd` of 32 dwords, and the epilogue.  The module copies the
; default data block from `DAT_00010113` in object 1 into `DAT_000f0002` in
; object 2.
;
; The module is assembly, because the original is library-style code: it holds
; `pushad`/`popad`, the segment register saves, and the `rep movsd`, and no EBP
; frame.  The two block addresses arrive as immediate operands that the loader
; relocates, so the rebuilt module differs from the original only in those two
; address fields.
.386
.model flat
public default_data_copy_
extrn _DAT_00010113:byte
extrn _DAT_000f0002:byte
.code
default_data_copy_ proc near
        pushad
        push ds
        push es
        push fs
        push gs
        mov esi, offset _DAT_00010113
        mov edi, offset _DAT_000f0002
        mov ecx, 0x20
        cld
        rep movsd
        pop gs
        pop fs
        pop es
        pop ds
        popad
        ret
default_data_copy_ endp
end
