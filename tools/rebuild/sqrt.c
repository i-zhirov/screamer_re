/* The rebuilt `sqrt` module.
 *
 * The original sits at linear 0x22bec in STARTH.EXE and holds 25 instructions
 * and 56 bytes up to the `ret 8` at 0x22c21.  The 12 padding bytes at
 * 0x22c24 through 0x22c2f and the next function at 0x22c30 follow the body.
 * A candidate record at 0x22bed sits on the `push edx` of the same prologue,
 * so the store keeps the 1-byte extent to that record and sets no separate
 * function size.  The identity check uses 25 instructions.
 *
 * The function takes one double on the stack.  A negative argument returns
 * 0.0.  Any other argument goes to the square-root core at 0xc6cbe and the
 * result is returned.  The core calls the hardware `fsqrt` for a non-negative
 * argument, and its negative path calls the error routine at 0xde484 with the
 * code 3.  The guard in this function keeps that error path unreachable.
 *
 * The name `sqrt` comes from the math name table at file offset 0xf8212: the
 * core at 0xc6cbe reports the code 3 to the error routine at 0xde484, and the
 * third name of the table is `sqrt`.  Many records call this function the
 * square-root wrapper, and the `sqrt_` stub of the `curve_speed` module
 * repeats the same negative guard.  The callers span the vehicle state chain,
 * the collision code, and the opponent code, so the record takes the section
 * Program functions.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The original has the EBP frame and one 8-byte local, so the module builds
 * with `-of+`.  The local FPU stub supplies the square-root core from the
 * hardware and the FPU run-time symbols.  The module uses no assembly
 * implementation.
 */
extern double sqrt_core(double value);

double sqrt(double value)
{
    double result;

    if (value < 0.0) {
        result = 0.0;
    } else {
        result = sqrt_core(value);
    }
    return result;
}
