/* The rebuilt `player_car_step_update` module.
 *
 * The original sits at linear 0x15ab8 in STARTH.EXE and holds 23 instructions
 * and 77 bytes up to the `ret` at 0x15b04.  The next entry starts at 0x15b08,
 * so the store record keeps the 80-byte extent and sets no separate function
 * size.
 *
 * The five view groups at 0x2003c, 0x201d0, 0x202c0, 0x20360, and 0x203c4
 * call this function on the player object at object-3 offset 0xf91c after the
 * player physics group at 0x15a80 and the effect group at 0x15bac.  The
 * function is the step group of the player car update.
 *
 * The object pointer arrives in `eax`.  The function runs eight player car
 * steps in order: the track node update at 0x1821c, the lap progress update at
 * 0x194a0, the body point collision detect at 0x11e74, the segment update at
 * 0x11f98, the segment apply at 0x16b98, the wheel contact update at 0x11a30,
 * the cornering update at 0x16ca0, and the contact update at 0x166a8.
 *
 * The call order, the object forward, and the step set support the name
 * `player_car_step_update`.  The original has no symbol, so the record keeps
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
extern void player_track_node_update(void *object);
extern void player_lap_progress_update(void *object);
extern void body_point_collision_detect(void *object);
extern void player_segment_update(void *object);
extern void player_segment_apply(void *object);
extern void wheel_contact_update(void *object);
extern void player_car_cornering_update(void *object);
extern void player_car_contact_update(void *object);

void player_car_step_update(void *object)
{
    void *volatile saved = object;
    player_track_node_update(saved);
    player_lap_progress_update(saved);
    body_point_collision_detect(saved);
    player_segment_update(saved);
    player_segment_apply(saved);
    wheel_contact_update(saved);
    player_car_cornering_update(saved);
    player_car_contact_update(saved);
}
