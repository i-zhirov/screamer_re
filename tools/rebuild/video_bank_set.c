/* The rebuilt `video_bank_set` module.
 *
 * The original sits at linear 0x8c0fc in STARTH.EXE and holds 10 instructions
 * and 24 bytes up to the `ret` at 0x8c113.  The next record starts at 0x8c150,
 * so the store record keeps the 84-byte extent and sets no separate function
 * size.  The extent also holds the sibling bank helpers at 0x8c114 and 0x8c12c.
 * The identity check uses 10 instructions.
 *
 * The bank index arrives in `al`.  The function zeroes `ebx`, loads the bank
 * shift `DAT_0007c18c` into `ecx`, shifts `al` left by `cl`, clears `ah`, copies
 * the zero-extended result to `edx`, sets `ax` to 0x4f05, and calls INT 10h.
 * `vesa_set_mode` at 0x8bdec stores the shift at 0x8be62 from the VBE mode-info
 * word at +4 and the mask 7.  INT 10h AX=4F05h sets the VBE display start, and
 * `dx` holds the start of the bank.
 *
 * The direct callers are the copy loop `video_copy` at 0x8beb8 with the call
 * sites at 0x8bed0, 0x8bf31, 0x8bf91, 0x8bff1, and 0x8c051, and the block at
 * 0x8c08c with the call site at 0x8c091.  `video_copy` passes the bank numbers
 * 0 through 4, and the block passes 0.  `video_copy` uses the name
 * `video_bank_set` for the callee, so the record keeps that name with the
 * confidence inferred.  The callers sit in the section race_loop callees, so
 * the record takes the same section.
 *
 * The original has no EBP frame and holds no floating-point instruction.  C
 * cannot inline INT 10h, so the module calls the `int386` helper with the VBE
 * function in `ax` and the bank start in `dx`; the module needs no assembly
 * source and no FPU stub.
 */
#include <i86.h>

extern int DAT_0007c18c;

void video_bank_set(unsigned char bank)
{
    union REGS regs;

    regs.w.ax = 0x4f05;
    regs.w.dx = (unsigned short)(unsigned char)(bank << DAT_0007c18c);
    int386(0x10, &regs, &regs);
}
