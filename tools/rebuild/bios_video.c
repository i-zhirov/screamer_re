/* The rebuilt `bios_video` module.
 *
 * The original sits at linear 0x108e4 in STARTH.EXE and holds 22 instructions: the
 * register-save prologue `push ebx`, `push ecx`, `push edx`, and `push ebp`, the frame
 * `mov ebp,esp` and `sub esp,0x2c`, the argument stored at `[ebp-4]`, the mode copied to
 * the first word of the general register set at `[ebp-0x2c]`, the four argument pointers
 * for the fill helper, the call at `0x4bf23`, the interrupt number `0x10`, the call at
 * `0x4bf4b`, and the epilogue.
 *
 * The helper at `0x4bf23` fills a 6-word segment register set from the live CPU state, so
 * the first pointer names the 0xc-byte local at `[ebp-0x10]`.  The helper at `0x4bf4b` takes
 * the interrupt number and then the register-set pointers, so the rebuild passes all four
 * pointers again.  The general register set at `[ebp-0x2c]` holds 0x1c bytes and starts with
 * the `ax` word, which is why the mode lands at `[ebp-0x2c]`.
 */
#include <i86.h>

extern void dpmi_fill_regs(struct SREGS *sregs, union REGS *regs, union REGS *regs2,
                          struct SREGS *sregs2);
extern void dpmi_call(int intnum);

void bios_video(int mode)
{
    union REGS regs;
    struct SREGS sregs;

    regs.w.ax = mode;
    dpmi_fill_regs(&sregs, &regs, &regs, &sregs);
    dpmi_call(0x10);
}
