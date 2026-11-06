/* The rebuilt `object_physics` module.
 *
 * The original sits at linear 0x1c810 in STARTH.EXE and holds 18 instructions
 * and 57 bytes: the frame, the argument slot, the guard on the state at
 * `+0x414`, the two gated steps, the two unconditional steps, and the
 * epilogue.
 *
 * The function runs the physics steps: when the state at `+0x414` is set, the
 * slide step and the gear state run, and then the gear step and the collision
 * step run.
 */
extern void object_slide_step(void *object);
extern void object_gear_state(void *object);
extern void object_gear_step(void *object);
extern void object_collision(void *object);

void object_physics(void *object)
{
    if (*(unsigned int *)((char *)object + 0x414) != 0) {
        object_slide_step(object);
        object_gear_state(object);
    }
    object_gear_step(object);
    object_collision(object);
}
