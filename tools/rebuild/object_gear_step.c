/* The rebuilt `object_gear_step` module.
 *
 * The original sits at linear 0x1d834 in STARTH.EXE and holds 133 instructions
 * and 635 bytes up to the first `ret`: the three register saves, the frame, the
 * 12-byte local area, the jump table at 0x1d824, the two initialization blocks,
 * the two integration blocks, the position compare, the counter, the two
 * vector clears, and the epilogue.  The record keeps the 772-byte extent.
 *
 * The function branches on the gear state at `+0x2f8`.  State 2 sets the state
 * to 1, the timer at `+0x2fc` to 3000.0, the counter at `+0x300` to 0, the two
 * velocities at `+0x304` and `+0x308` to 12.0, and the velocity at `+0x30c` to
 * 0, then falls through to state 1.  State 1 adds the scaled timer to the
 * position at `+0x49c`, damps the three velocities with the coefficient at
 * `DAT_0010cf08 + 0x184`, integrates them into `+0x314`, `+0x318`, and
 * `+0x31c`, and subtracts 6000.0 times the frame delta from the timer.
 *
 * When the position stays at or below the target at `+0x238`, the function
 * increments the counter, snaps the position to the target, reflects the timer
 * by 0.4, clears the three integrated values, sets the velocity at `+0x30c`
 * from the counter parity and 0.6, sets the velocity at `+0x304` to -1.0, and
 * clears the velocity at `+0x308`.  When the counter passes 3, it clears the
 * three integrated values and the state.
 *
 * State 3 sets the state to 4 and the velocity at `+0x308` to 24.0, then falls
 * through to state 4.  State 4 damps that velocity, integrates it into
 * `+0x318`, and clears the value and the state when the value reaches the
 * threshold 12.566371.
 *
 * The original reads the four constants from object 3.  The harness provides
 * the four globals with the exact bit patterns.
 *
 * The original uses a jump table at `0x1d824`.  The rebuild uses an if-else
 * chain, because the V2 jump table at the start of the code object faults in
 * the harness.  The same fault shows in the earlier switch modules.
 *
 * The single conversion to an integer uses the Watcom helper at 0x4d9f4,
 * which truncates toward zero.  The module uses a local truncating FPU stub,
 * because the shared stub rounds to nearest.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern float DAT_00118e04;
extern unsigned int DAT_0010cf08;
extern float gear_decrement;
extern double gear_bounce;
extern double gear_steer;
extern float gear_wrap;

void object_gear_step(char *object)
{
    int state;
    int sign;

    state = *(int *)(object + 0x2f8);
    if (state == 2) {
        *(int *)(object + 0x2f8) = 1;
        *(float *)(object + 0x2fc) = 3000.0f;
        *(int *)(object + 0x300) = 0;
        *(float *)(object + 0x304) = 12.0f;
        *(int *)(object + 0x30c) = 0;
        *(float *)(object + 0x308) = 12.0f;
        state = 1;
    }
    if (state == 1) {
        *(int *)(object + 0x49c) = (int)(*(float *)(object + 0x2fc)
            * DAT_00118e04 + *(int *)(object + 0x49c));
        *(float *)(object + 0x304) = *(float *)(object + 0x304)
            * *(float *)(DAT_0010cf08 + 0x184);
        *(float *)(object + 0x314) = *(float *)(object + 0x314)
            + *(float *)(object + 0x304) * DAT_00118e04;
        *(float *)(object + 0x308) = *(float *)(object + 0x308)
            * *(float *)(DAT_0010cf08 + 0x184);
        *(float *)(object + 0x318) = *(float *)(object + 0x318)
            + *(float *)(object + 0x308) * DAT_00118e04;
        *(float *)(object + 0x30c) = *(float *)(object + 0x30c)
            * *(float *)(DAT_0010cf08 + 0x184);
        *(float *)(object + 0x31c) = *(float *)(object + 0x31c)
            + *(float *)(object + 0x30c) * DAT_00118e04;
        *(float *)(object + 0x2fc) = *(float *)(object + 0x2fc)
            - DAT_00118e04 * gear_decrement;
        if (*(int *)(object + 0x49c) <= *(int *)(object + 0x238)) {
            *(int *)(object + 0x300) = *(int *)(object + 0x300) + 1;
            *(int *)(object + 0x49c) = *(int *)(object + 0x238);
            *(float *)(object + 0x2fc) = -(*(float *)(object + 0x2fc)
                * gear_bounce);
            *(float *)(object + 0x318) = 0.0f;
            *(float *)(object + 0x31c) = *(float *)(object + 0x318);
            *(float *)(object + 0x314) = *(float *)(object + 0x31c);
            if (*(int *)(object + 0x300) & 1) {
                sign = -1;
            } else {
                sign = 1;
            }
            *(float *)(object + 0x30c) = sign * gear_steer;
            *(float *)(object + 0x304) = -1.0f;
            *(float *)(object + 0x308) = 0.0f;
        }
        if (*(int *)(object + 0x300) > 3) {
            *(float *)(object + 0x31c) = 0.0f;
            *(float *)(object + 0x318) = *(float *)(object + 0x31c);
            *(float *)(object + 0x314) = *(float *)(object + 0x318);
            *(int *)(object + 0x2f8) = 0;
        }
    }
    if (state == 3) {
        *(int *)(object + 0x2f8) = 4;
        *(float *)(object + 0x308) = 24.0f;
        state = 4;
    }
    if (state == 4) {
        *(float *)(object + 0x308) = *(float *)(object + 0x308)
            * *(float *)(DAT_0010cf08 + 0x184);
        *(float *)(object + 0x318) = *(float *)(object + 0x318)
            + *(float *)(object + 0x308) * DAT_00118e04;
        if (*(float *)(object + 0x318) >= gear_wrap) {
            *(float *)(object + 0x318) = 0.0f;
            *(int *)(object + 0x2f8) = 0;
        }
    }
}
