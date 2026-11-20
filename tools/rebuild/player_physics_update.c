/* The rebuilt `player_physics_update` module.
 *
 * The original sits at linear 0x15a80 in STARTH.EXE and holds 17 instructions
 * and 53 bytes up to the `ret` at 0x15ab4.  The next entry starts at 0x15ab8,
 * so the store record keeps the 56-byte extent and sets no separate function
 * size.  The identity check uses 17 instructions.
 *
 * The five view groups at 0x2003c, 0x201d0, 0x202c0, 0x20360, and 0x203c4
 * call this function on the player object at object-3 offset 0xf91c before
 * the effect group at 0x15bac and the step group at 0x15ab8.  The function is
 * the physics group of the player car update.
 *
 * The object pointer arrives in `eax`.  The function runs five player steps in
 * order: the race clock update at 0x18a14, the engine speed update at 0x18db0,
 * the throttle update at 0x160a4, the brake update at 0x16150, and the
 * steering update at 0x1629c.
 *
 * The call order, the object forward, and the step set support the name
 * `player_physics_update`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original stores the argument to the frame and reloads it before each
 * call (`sub esp,4`, `mov [ebp-4],eax`, `mov eax,[ebp-4]`).  A plain parameter
 * stays in a preserved register and produces no load.  The volatile local
 * reproduces the store and the reload, as `error_report.c` records for the
 * same frame-local pattern.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and makes no math call, so the
 * module needs no FPU stub, and the module uses no assembly implementation.
 */
extern void race_clock_update(void *object);
extern void engine_rpm_update(void *object);
extern void player_throttle_update(void *object);
extern void player_brake_update(void *object);
extern void player_steering_update(void *object);

void player_physics_update(void *object)
{
    void *volatile saved = object;
    race_clock_update(saved);
    engine_rpm_update(saved);
    player_throttle_update(saved);
    player_brake_update(saved);
    player_steering_update(saved);
}
