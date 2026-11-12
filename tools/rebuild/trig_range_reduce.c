/* The rebuilt `trig_range_reduce` module.
 *
 * The original sits at linear 0x4da3a in STARTH.EXE.  The body holds 22
 * instructions and 44 bytes up to the `ret` at 0x4da65.  The next helper starts
 * at 0x4da66, so the store record keeps the 44-byte extent and sets no separate
 * function size.  The identity check uses 22 instructions.
 *
 * The two direct callers are the cosine core at 0x4da26 and the sine core at
 * 0x4da30.  Each core computes `fcos` or `fsin` and then calls this helper.  The
 * helper reads the x87 status word.  When the C2 bit is clear it returns with
 * the carry flag set.  When the C2 bit is set it loads the 80-bit 2 pi constant
 * at object offset 0x3da1c, computes the remainder with `fprem`, and repeats
 * through the `fnstsw` test until the reduction is complete.  It clears the
 * carry flag and returns.  The core repeats while the carry flag is clear, so a
 * value out of the `fsin` or `fcos` range is reduced one turn at a time and then
 * tested again.
 *
 * The data flow reduces the value modulo 2 pi and keeps the sign of the value,
 * as `fprem` does.  The C form computes that remainder with a subtraction loop.
 * The x87 C2 gate and the carry-flag result belong to the core loop and have no
 * C form, so the rebuilt bytes differ and the record takes the status `rebuilt`.
 *
 * The cosine helper at 0x4da66 and the sine helper at 0x4da72 load the double
 * argument and call the two cores.  This helper is shared by both, so the module
 * takes the section Program functions.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The 2 pi
 * constant arrives through the global `trig_two_pi`, because the harness image
 * does not load the constant segment.  The shared `fpu_stub.asm` supplies the
 * FPU run-time symbols and the float-used marker.
 */
extern double trig_two_pi;

double trig_range_reduce(double value)
{
    double result;

    result = value;
    while (result >= trig_two_pi) {
        result -= trig_two_pi;
    }
    while (result <= -trig_two_pi) {
        result += trig_two_pi;
    }
    return result;
}
