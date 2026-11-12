/* The rebuilt `player_effect_state_set` module.
 *
 * The original sits at linear 0x19478 in STARTH.EXE and holds 14 instructions
 * and 40 bytes up to the `ret` at 0x1949f.  The record keeps the 40-byte
 * extent and sets no separate function size.  The identity check uses 14
 * instructions.
 *
 * The two direct callers run on the player object.  `player_car_cornering_update`
 * at 0x16ca0 calls this function at 0x16da4 with the state 2 when the car
 * skids and the effect control word is zero.  `engine_rpm_update` at 0x18db0
 * calls this function at 0x19044 with the state 5 when the engine speed
 * exceeds the limit and the joystick sits at an extreme.  The callers sit in
 * the section The vehicle state chain, so the record takes the same section.
 *
 * The function stores the first argument in `eax` and the second in `edx`, the
 * Watcom register calling convention.  It tests the state field `+0x148` of
 * the object.  A zero field takes the value and a non-zero field keeps its
 * value, so a running effect is not replaced.  The car state values 2 and 5
 * and the two callers support the name `player_effect_state_set`.  The
 * original has no symbol, so the record keeps the name with the confidence
 * inferred.
 *
 * The original stores both arguments to the frame and reloads them
 * (`sub esp,8`, `mov [ebp-8],eax`, `mov [ebp-4],edx`, then the loads).  A
 * plain parameter stays in a preserved register and produces no store.  The
 * two volatile locals reproduce the store and the reload, as `error_report.c`
 * records for the same frame-local pattern.
 *
 * The original has the EBP frame and two locals, so the module builds with
 * `-of+`.  The function holds no floating-point instruction and calls no
 * helper, so the module needs no FPU stub and no assembly implementation.
 */
struct player_object {
    unsigned char pad[0x148];
    int effect_state;
};

void player_effect_state_set(struct player_object *object, int state)
{
    struct player_object *volatile saved_object = object;
    int volatile saved_state = state;

    if (saved_object->effect_state == 0) {
        saved_object->effect_state = saved_state;
    }
}
