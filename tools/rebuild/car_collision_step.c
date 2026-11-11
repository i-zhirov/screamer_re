/* The rebuilt `car_collision_step` module.
 *
 * The original sits at linear 0x1c7d0 in STARTH.EXE and holds 19 instructions
 * and 61 bytes up to the `ret` at 0x1c80c.  The next function `object_physics`
 * starts at 0x1c810, so the store record keeps the 64-byte extent and sets no
 * separate function size.  The identity check uses 19 instructions.
 *
 * The only direct caller at 0x20182 is `race_simulation_update` at 0x2003c.
 * It runs this step on each 0x590-byte car record at `DAT_00110ab8` after
 * `object_prepare`.  The step runs the nearby mark at 0x126e4, the collision
 * pass at 0x1d00c, and the opponent avoidance at 0x1e414.  The caller, the
 * three callees, and the car record support the name `car_collision_step`.
 * The original has no symbol, so the record keeps the name with the
 * confidence inferred.  The step sits in the object update chain, so the
 * record takes the section `object update callees`.
 *
 * The function returns at once when the crash state at `+0x2f8` is not zero
 * or when the physics gate at `+0x414` is zero.  Otherwise it runs the three
 * collision passes in order with the record in `eax`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no direct call of its own,
 * so the module needs no FPU stub and no assembly.
 */
extern void car_nearby_mark(void *object);
extern void car_collision_pass(void *object);
extern void opponent_avoid_player(void *object);

void car_collision_step(void *object)
{
    if (*(unsigned int *)((char *)object + 0x2f8) != 0) {
        return;
    }
    if (*(unsigned int *)((char *)object + 0x414) == 0) {
        return;
    }
    car_nearby_mark(object);
    car_collision_pass(object);
    opponent_avoid_player(object);
}
