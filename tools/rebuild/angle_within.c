/* The rebuilt `angle_within` module.
 *
 * The original sits at linear 0x22664 in STARTH.EXE.  It holds 32 instructions
 * and 85 bytes up to the `ret 0xc` at 0x226b6.  The next function
 * `direction_angle` starts at 0x226bc, so the store record keeps the 88-byte
 * extent and sets no separate function size.  The identity check uses 32
 * instructions.
 *
 * The function takes two angles and a tolerance on the stack.  It forms the
 * absolute difference, reduces it by one full turn while it is larger than
 * 2 pi, and returns 1 when the reduced difference lies within the tolerance of
 * zero or of one full turn.  The two cases are the same position on the
 * circle, so the function tells whether the two angles agree within the
 * tolerance.
 *
 * The only direct caller is `car_sound_update` at 0x242bc.  It passes the car
 * angle `+0x13c` plus a bias, the heading `+0x4cc`, and the tolerance pi / 4.
 * The caller and the nearby angle helpers belong to the vehicle state chain,
 * so the record takes the section The vehicle state chain.
 *
 * The original has no symbol, so the name comes from the data flow.  The
 * caller and the harness already use the name `angle_within`, so the record
 * keeps that name with the confidence inferred.
 *
 * The intrinsic `fabs` gives the inline x87 absolute value, as the original
 * does.  The two accept and reject labels keep the one shared return block in
 * the original order.  The constants 2 pi and -2 pi arrive through globals,
 * because the minimal image does not load the constant segment.  The original
 * has the EBP frame, so the module builds with `-of+`.  The shared
 * `fpu_stub.asm` supplies the FPU run-time symbols and the float-used marker.
 * The module uses no assembly implementation.
 */
extern double fabs(double value);
#pragma intrinsic (fabs)

extern float angle_turn;
extern float angle_turn_neg;

int angle_within(float a, float b, float tolerance)
{
    float difference;

    difference = (float)fabs((double)(a - b));
    while (difference > angle_turn) {
        difference = difference + angle_turn_neg;
    }
    if (difference <= tolerance) {
        goto accept;
    }
    if (angle_turn - tolerance > difference) {
        goto reject;
    }
accept:
    return 1;
reject:
    return 0;
}
