/* The rebuilt `dpmi_alloc_dos` module.
 *
 * The original sits at linear 0x8be93 in STARTH.EXE.  The entry path holds 11
 * instructions and 28 bytes up to the `ret` at 0x8beae.  The failure branch at
 * 0x8beaf holds the `mov eax,1` and the `ret`, and the next function starts at
 * 0x8beb8, so the store record keeps the 37-byte extent and sets no separate
 * function size.  The identity check uses 11 instructions.
 *
 * The only direct caller is `vesa_set_mode` at 0x8bdec with the call site at
 * 0x8bdf7.  It passes the size 0x100 in `eax`, tests the status in `eax`, and
 * uses the block address in `edi`.  The caller stores the address at
 * `DAT_0007c188` and runs the VBE mode info call against the block.
 *
 * The entry copies the size from `eax` to `ebx` and sets the DPMI function
 * 0x0100 in `eax`.  It shifts the size into paragraphs and adds one, so the
 * paragraph count is `(size >> 4) + 1`.  It runs INT 31h and branches on the
 * carry flag: on a carry it returns 1, and otherwise it masks the segment to 16
 * bits, shifts it into a linear address, stores the address in `edi`, and
 * returns 0.
 *
 * The function is the inline DOS memory allocator of the vesa_set_mode path.
 * The caller and the data flow support the name `dpmi_alloc_dos`.  The original
 * has no symbol, so the record keeps the name with the confidence inferred.
 * The caller sits in the section Subsystem initialisation, so the record takes
 * the same section.
 *
 * The C rebuild passes the linear address through the second argument, because
 * C cannot return a second value in `edi`.  The status stays the return value.
 * The paragraph count and the INT 31h function stay the same.
 *
 * The original has no EBP frame and holds no floating-point instruction.  The
 * interrupt stays in one `#pragma aux` helper, so the module needs no assembly
 * source and no FPU stub.  The helper holds no `ret`, because the compiler
 * inlines the body into this source, and a `ret` would return from the whole
 * function.
 */

static unsigned int dpmi_alloc_block(unsigned int paragraphs,
                                     unsigned int *status);
#pragma aux dpmi_alloc_block = \
    "mov esi,edx" \
    "mov ebx,eax" \
    "mov eax,0x100" \
    "int 0x31" \
    "mov ecx,0" \
    "adc ecx,0" \
    "movzx eax,ax" \
    "mov dword ptr [esi],ecx" \
    parm [eax] [edx] value [eax] modify exact [eax ebx ecx edx esi];

unsigned int dpmi_alloc_dos(unsigned int size, unsigned int *linear)
{
    unsigned int paragraphs;
    unsigned int segment;
    unsigned int status;

    paragraphs = (size >> 4) + 1;
    segment = dpmi_alloc_block(paragraphs, &status);
    if (status != 0) {
        return 1;
    }
    *linear = segment << 4;
    return 0;
}
