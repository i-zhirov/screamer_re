; The rebuilt `vesa_set_mode` module.
;
; The original sits at linear 0x8bdec in STARTH.EXE and holds 51 instructions
; and 167 bytes.  The module saves the mode argument at `DAT_0007c17c`, allocates
; a DOS memory block below 1 MB through `dpmi_alloc_dos`, builds the real-mode
; register set at `DAT_0007c190` for the VBE mode info call (INT 10h AX=4F01h,
; CX=101h), runs it through the DOS/4GW real-mode interrupt 31h AX=0300h, stores
; the memory model and the window bits from the info block, and sets the mode
; through INT 10h AX=13h or AX=4F02h.
;
; The module is assembly, because the original is library-style code: it holds
; `pushad`/`popad`, the direct INT 31h and INT 10h calls, and no EBP frame.  The
; mode argument arrives in `eax`, and the return value is 0 on success and 1 on
; an allocation or a VBE failure.
.386
.model flat
public vesa_set_mode_
extrn _DAT_0007c17c:dword
extrn _DAT_0007c180:dword
extrn _DAT_0007c188:dword
extrn _DAT_0007c18c:dword
extrn _DAT_0007c190:byte
extrn _DAT_0007c1a8:dword
extrn _DAT_0007c1ac:dword
extrn _DAT_0007c1b2:word
extrn dpmi_alloc_dos_:near
.code
vesa_set_mode_ proc near
        pushad
        mov dword ptr [_DAT_0007c17c], eax
        mov eax, 0x100
        call dpmi_alloc_dos_
        test eax, eax
        je alloc_ok
        popad
        ret
alloc_ok:
        mov dword ptr [_DAT_0007c188], edi
        mov edi, dword ptr [_DAT_0007c188]
        mov ebx, 0x101
        mov eax, edi
        shr eax, 4
        mov word ptr [_DAT_0007c1b2], ax
        and edi, 0xf
        mov dword ptr [_DAT_0007c190], edi
        mov ax, 0x4f01
        mov dword ptr [_DAT_0007c1ac], eax
        mov dword ptr [_DAT_0007c1a8], ebx
        mov bl, 0x10
        mov ax, 0x300
        mov bh, 0
        mov cx, 0
        push ds
        pop es
        mov edi, offset _DAT_0007c190
        int 0x31
        xor eax, eax
        mov esi, dword ptr [_DAT_0007c188]
        mov al, byte ptr [esi + 0x1a]
        mov dword ptr [_DAT_0007c180], eax
        mov ax, word ptr [esi + 4]
        and eax, 7
        mov dword ptr [_DAT_0007c18c], eax
        mov eax, dword ptr [_DAT_0007c17c]
        test eax, eax
        jne vbe_mode
        mov ax, 0x13
        int 0x10
        jmp mode_ok
vbe_mode:
        mov ax, 0x4f02
        mov bx, 0x101
        int 0x10
        cmp ax, 0x4f
        je mode_ok
        mov eax, 1
        popad
        ret
mode_ok:
        xor eax, eax
        popad
        ret
vesa_set_mode_ endp
end
