/* The rebuilt `gameport_pulse` module.
 *
 * The original sits at linear 0x1028c in STARTH.EXE and holds 18 instructions
 * and 36 bytes up to the `ret` at 0x102af.  The next function starts at
 * 0x102b0, so the store record keeps the 36-byte extent and sets no separate
 * function size.  The identity check uses 18 instructions.
 *
 * The only direct caller is `joystick_axis_read` at 0x10263, at the call site
 * 0x10274.  It passes the axis mask in `eax` and the game port in `edx`, and it
 * also builds a selector in `ebx` that the helper ignores.  The caller keeps
 * the low 12 bits of the count, so the result is the game port pulse length.
 *
 * The function copies the mask from `al` to `ah` and waits up to 0x1000 reads
 * for the mask bits to clear.  It then clears the counter, writes the last read
 * byte to the port, and counts the reads until the mask bits clear again or the
 * count reaches 0x2000.  The original disables the interrupts between the write
 * and the count loop with `cli` and `sti`.  The C rebuild leaves the guard out,
 * because the harness link holds no `_disable` and no `_enable` helper.  It
 * returns the count.
 *
 * The caller, the game port, and the pulse timing support the name
 * `gameport_pulse`.  The original has no symbol, so the record keeps the name
 * with the confidence inferred.  The caller sits in the section race_loop
 * callees, so the record takes the same section.
 *
 * The original has no EBP frame and holds no floating-point instruction, so
 * the module builds without `-of+` and needs no FPU stub and no assembly body.
 */
#include <i86.h>

extern unsigned char inportb(unsigned short port);
extern void outportb(unsigned short port, unsigned char value);

unsigned int gameport_pulse(unsigned int mask, unsigned int port,
                            unsigned int selector)
{
    unsigned char bit;
    unsigned char value;
    unsigned int wait;
    unsigned int count;

    (void)selector;
    bit = (unsigned char)mask;
    wait = 0x1000;
    do {
        value = inportb((unsigned short)port);
        if ((value & bit) == 0) {
            break;
        }
        wait = wait - 1;
    } while (wait != 0);

    count = 0;
    outportb((unsigned short)port, value);
    do {
        value = inportb((unsigned short)port);
        if ((value & bit) == 0) {
            break;
        }
        count = count + 1;
    } while (count != 0x2000);

    return count;
}
