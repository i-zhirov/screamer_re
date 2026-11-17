/* The rebuilt `video_selector_setup` module.
 *
 * The original sits at linear 0x8bc7c in STARTH.EXE and holds 33 instructions
 * and 116 bytes up to the `ret` at 0x8bcef.  The next recorded function starts
 * at 0x8bd54, so the store record keeps the 216-byte extent and sets no
 * separate function size.  The identity check uses 33 instructions.
 *
 * The only direct caller is `screen_mode_set` at 0x4c850 with the call site at
 * 0x4c8e6.  That function writes the geometry fields of the screen mode record
 * and then calls this function.  The caller sits in the section `Subsystem
 * initialisation`, so the record takes that section.  The caller already uses
 * the name `video_selector_setup` for the callee, so the record keeps that name
 * with the confidence inferred.  The original has no symbol.
 *
 * The original saves all general registers and the `ds`, `es`, `fs`, and `gs`
 * segment registers.  It sets `ds` from the selector at `cs:[0x214d4]`, reads
 * the global descriptor table register with `sgdt` into `0x7bcf8`, and moves
 * the table base from `0x7bcfa` to `0x21480`.
 *
 * The original then loads the `fs` selector from `0x21490` and rounds the same
 * dword down to a multiple of eight.  It adds the rounded value and the constant
 * 2 to the table base and stores the descriptor address at `0x7bcf0`.
 *
 * The function calls six helper blocks.  Each helper reads the word at
 * `0x7a7c4` and patches the selector immediates of its code block at four
 * sites.  The six blocks are at 0xdbca0, 0xd68a0, 0xdcbc0, 0xd1d24, 0xda7b8,
 * and 0xccde0.
 *
 * The tail copies the dword at `0x214ec` to `0x73664` and `0x7abd3`.
 *
 * The flat target keeps one data segment, so the rebuild omits the segment
 * register loads.  The rebuild reads the table base from the model global
 * `DAT_0007bcfa`.  The original has no EBP frame and uses no floating-point
 * instruction, so the module builds without `-of+` and links no FPU stub.  The
 * module uses no assembly body.
 */
extern unsigned long DAT_0007bcfa;
extern unsigned long DAT_00021490;
extern unsigned long DAT_00021480;
extern unsigned long DAT_0007bcf0;
extern unsigned long DAT_000214ec;
extern unsigned long DAT_00073664;
extern unsigned long DAT_0007abd3;

extern void selector_patch_a(void);
extern void selector_patch_b(void);
extern void selector_patch_c(void);
extern void selector_patch_d(void);
extern void selector_patch_e(void);
extern void selector_patch_f(void);

void video_selector_setup(void)
{
    unsigned long base;
    unsigned long offset;

    base = DAT_0007bcfa;
    DAT_00021480 = base;
    offset = DAT_00021490 >> 3;
    offset = offset << 3;
    DAT_0007bcf0 = base + offset + 2;
    selector_patch_a();
    selector_patch_b();
    selector_patch_c();
    selector_patch_d();
    selector_patch_e();
    selector_patch_f();
    DAT_00073664 = DAT_000214ec;
    DAT_0007abd3 = DAT_000214ec;
}
