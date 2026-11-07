/* The rebuilt `player_car_spin_update` module.
 *
 * The original sits at linear 0x188b8 in STARTH.EXE and holds 99 instructions
 * and 347 bytes up to the `ret` at 0x18a12.  The next function starts at
 * 0x18a14, so the record keeps the 348-byte extent and sets no function size.
 *
 * The only direct caller is the effect group at 0x15bac, which calls this
 * function at 0x15bd8 after `player_car_reset_update` at 0x18528.  The five
 * view groups at 0x2003c, 0x201d0, 0x202c0, 0x20360, and 0x203c4 call the
 * effect group on the player object at object-3 offset 0xf91c, so the module
 * takes the section The vehicle state chain.
 *
 * The function is a state machine on the counter `+0x170`.  The skid update at
 * 0x17d14 sets the counter to 1 when the slip gate opens.  `player_car_link`
 * at 0x15c1c adds the offset `+0x128` to the render yaw `+0x4cc`, so the
 * offset is a render angle.
 *
 *     state 1   the start: the sign of the yaw step `+0x2f4` selects a full
 *               turn of +2*pi or -2*pi at `+0x128`, and the counter becomes 4
 *               for a negative yaw step or 2 for the other case
 *     state 2   the negative turn unwinds: `+0x128` adds the step, and a
 *               crossing above zero clears the offset and the counter
 *     state 3   the hold: the offset stays unchanged
 *     state 4+  the positive turn unwinds: `+0x128` subtracts the step, and a
 *               crossing below zero clears the offset and the counter
 *
 * The step is the frame delta `DAT_00118e04` times `(abs(+0x128) + 0.2) / pi`
 * times 3.0.  The constants arrive through globals, because the minimal image
 * does not load the constant segment.  The function converts the counter and
 * the sign to an integer through the Watcom helper at 0x4d9f4, which truncates
 * toward zero, so the local FPU stub makes `__CHP` truncate.
 *
 * The full turn and the render-yaw use support the name player_car_spin_update.
 * The original has no symbol, so the record keeps the name with the confidence
 * inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern float DAT_00118e04;
extern float player_car_spin_turn;
extern float player_car_spin_scale;
extern double player_car_spin_bias;
extern double player_car_spin_pi;

static float absolute_float(float value)
{
    return value < 0.0f ? -value : value;
}

void player_car_spin_update(void *object)
{
    char *base = (char *)object;
    float sign;
    float rate;
    int state;
    int flag;

    state = *(int *)(base + 0x170);
    if (state == 1) {
        sign = *(float *)(base + 0x2f4) < 0.0f ? 1.0f : -1.0f;
        *(int *)(base + 0x170) = (int)(sign + player_car_spin_turn);
        *(float *)(base + 0x128) =
            *(float *)(base + 0x128) + sign * player_car_spin_scale;
        return;
    }
    if (state <= 1) {
        return;
    }

    sign = (float)(state - 3);
    rate = (float)(((double)absolute_float(*(float *)(base + 0x128))
                    + player_car_spin_bias) / player_car_spin_pi);
    flag = 0;
    if (sign > 0.0f) {
        *(float *)(base + 0x128) = *(float *)(base + 0x128)
            - DAT_00118e04 * rate * player_car_spin_turn;
        if (*(float *)(base + 0x128) < 0.0f) {
            flag = 1;
        }
    }
    if (sign < 0.0f) {
        *(float *)(base + 0x128) = *(float *)(base + 0x128)
            + DAT_00118e04 * rate * player_car_spin_turn;
        if (*(float *)(base + 0x128) > 0.0f) {
            flag = 1;
        }
    }
    if (flag != 0) {
        *(float *)(base + 0x128) = 0.0f;
        *(int *)(base + 0x170) = 0;
    }
}
