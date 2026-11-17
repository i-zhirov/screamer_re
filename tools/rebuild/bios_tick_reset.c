/* The rebuilt `bios_tick_reset` module.
 *
 * The original sits at linear 0x1f990 in STARTH.EXE and holds 34 instructions
 * and 115 bytes up to the `ret` at 0x1fa02.  A padding byte at 0x1fa03 precedes
 * the next function at 0x1fa04, so the store record keeps the 116-byte extent
 * and sets no separate function size.  The identity check uses 34 instructions.
 *
 * The function holds no direct caller and no data reference, so the record
 * infers the name from the body.  The body fills the segment set at `0xcf34`
 * through `dpmi_fill_regs` at `0x4bf23` and runs the BIOS timer service through
 * `dpmi_call` at `0x4bf4b`.
 *
 * The register block at `0xcf18` holds EAX at offset 0 and EDX at offset 0xc.
 * The function clears EAX and calls INT 1Ah with AH=0, which reads the tick
 * count in CX:DX.  It saves the low word from EDX, then writes EAX=0x0100,
 * ECX=0, and EDX=0 and calls INT 1Ah with AH=1, which sets the tick count to
 * zero.  It returns the saved low word.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so it needs no FPU stub.  The
 * module uses no assembly body.
 */
struct dpmi_regs {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
    unsigned int esi;
    unsigned int edi;
    unsigned int flags;
};

struct dpmi_sregs {
    unsigned short es;
    unsigned short cs;
    unsigned short ss;
    unsigned short ds;
    unsigned short fs;
    unsigned short gs;
};

extern struct dpmi_regs DAT_0000cf18;
extern struct dpmi_sregs DAT_0000cf34;
extern void dpmi_fill_regs(struct dpmi_sregs *sregs);
extern int dpmi_call(int intnum, struct dpmi_regs *regs,
                     struct dpmi_regs *result, struct dpmi_sregs *sregs);

unsigned int bios_tick_reset(void)
{
    unsigned int saved;

    DAT_0000cf18.eax = 0;
    dpmi_fill_regs(&DAT_0000cf34);
    dpmi_call(0x1a, &DAT_0000cf18, &DAT_0000cf18, &DAT_0000cf34);
    saved = DAT_0000cf18.edx & 0xffff;
    DAT_0000cf18.eax = 0x100;
    DAT_0000cf18.edx = 0;
    DAT_0000cf18.ecx = 0;
    dpmi_call(0x1a, &DAT_0000cf18, &DAT_0000cf18, &DAT_0000cf34);
    return saved;
}
