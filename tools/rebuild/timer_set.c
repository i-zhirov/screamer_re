/* The rebuilt `timer_set` module.
 *
 * The original sits at linear 0x101f6 in STARTH.EXE and holds 13 instructions
 * and 23 bytes up to the `ret` at 0x1020c.  The next record starts at 0x10257,
 * so the store record keeps the 97-byte extent and sets no separate function
 * size.  The extent also holds the keyboard and BIOS helpers at 0x101e9,
 * 0x1020d, and 0x1021a.  The identity check uses 13 instructions.
 *
 * The divisor arrives in `eax`.  The function disables the interrupts, copies
 * the divisor to `ecx`, writes the mode byte 0x36 (channel 0, lobyte/hibyte
 * access, mode 3, binary) to the command register 0x43, writes the low byte to
 * channel 0 at 0x40, writes the high byte to the same port, enables the
 * interrupts, and returns.  The three `jmp short` instructions give the ISA bus
 * time between the two writes.
 *
 * The only direct caller is `dos_datetime_set` at 0x1fa2c with the call site
 * at 0x1fa3c.  It passes the divisor 0, so the timer keeps the default period
 * of 65536 counts.  The caller names the callee `timer_set`, so the record
 * keeps that name with the confidence inferred.  The caller sits in the
 * section Subsystem initialisation, so the record takes the same section.
 *
 * The original has no EBP frame and no floating-point instruction.  C cannot
 * emit the `cli`, the `sti`, or the inline port writes.  The module calls the
 * `outportb` helper and leaves out the interrupt guard.  The module needs no
 * `-of+`, no assembly source, and no FPU stub.
 */
#include <i86.h>

extern void outportb(unsigned short port, unsigned char value);

void timer_set(unsigned int divisor)
{
    outportb(0x43, 0x36);
    outportb(0x40, (unsigned char)divisor);
    outportb(0x40, (unsigned char)(divisor >> 8));
}
