/* The rebuilt `joystick_axis_read` module.
 *
 * The original sits at linear 0x10263 in STARTH.EXE and holds 16 instructions
 * and 41 bytes up to the `ret` at 0x1028b.  The next function starts at
 * 0x1028c, so the store record keeps the 41-byte extent and sets no separate
 * function size.  The identity check uses 16 instructions.
 *
 * The only direct caller is `joystick_update` at 0x24da0, which calls this
 * function at 0x24e77 for a control record of kind 1.  It passes the axis mask
 * `1 << device` in `eax` and the game port from `+0x10` of the axis record in
 * `edx`.  The caller keeps the low 12 bits of the result and scales the value,
 * so the result holds the axis timing.
 *
 * The function first builds the byte `((port - 0x200) << 1) + mask` with the
 * low bit cleared and passes it as the third argument.  The helper at 0x1028c
 * measures the pulse on the game port and returns the count.  The function
 * then reads the same port again for the four button bits, shifts them to bits
 * 16 through 19, and joins the low 12 timing bits.  The original keeps the
 * third argument in `ebx` and the helper ignores it, but the argument stays in
 * the interface so the rebuilt code keeps the same shape.
 *
 * The game port, the axis mask, and the caller support the name
 * `joystick_axis_read`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.  The caller sits in the section Program
 * functions, so the record takes the same section.
 *
 * The original has no EBP frame and holds no floating-point instruction, so
 * the module builds without `-of+` and needs no FPU stub and no assembly body.
 */
#include <i86.h>

extern unsigned int gameport_pulse(unsigned int mask, unsigned int port,
                                   unsigned int selector);
extern unsigned char inportb(unsigned short port);

unsigned int joystick_axis_read(unsigned int mask, unsigned int port)
{
    unsigned int selector;
    unsigned int count;
    unsigned int buttons;

    selector = ((port - 0x200) << 1) + mask;
    selector &= ~1u;
    count = gameport_pulse(mask, port, selector);
    buttons = inportb((unsigned short)port);
    return ((buttons & 0xf0) << 12) | (count & 0xfff);
}
