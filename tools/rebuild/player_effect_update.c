/* The rebuilt `player_effect_update` module.
 *
 * The original sits at linear 0x15b08 in STARTH.EXE and holds 18 instructions
 * and 57 bytes up to the `ret` at 0x15b40.  The next function starts at
 * 0x15b44, so the store record keeps the 60-byte extent and sets no separate
 * function size.
 *
 * The player update at 0x2003c runs this group at 0x20154 on the player object
 * at object-3 offset 0xf91c.  The player nearby mark at 0x12378 records this
 * group as its only caller, and the car proximity pass at 0x17408 records the
 * same.  The sibling traffic group at 0x15b44 has the same shape with the
 * traffic nearby marker and the traffic collision pass.  The function is the
 * player effect group of the vehicle update.
 *
 * The object pointer arrives in `eax`.  The function first places the player
 * body points through 0x17c6c.  It returns at once when the state field
 * `+0x148` is not zero.  Otherwise it marks the nearby car records at 0x12378,
 * runs the car proximity pass at 0x17408, and starts the nearby opponents at
 * 0x124c0, all with the same player object.
 *
 * The player update caller, the player steps, and the state gate support the
 * name `player_effect_update`.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
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
extern void player_body_point_place(void *object);
extern void player_nearby_mark(void *object);
extern void car_proximity_pass(void *object);
extern void opponent_start(void *object);

void player_effect_update(void *object)
{
    void *volatile saved = object;

    player_body_point_place(saved);
    if (*(int *)((char *)saved + 0x148) == 0) {
        player_nearby_mark(saved);
        car_proximity_pass(saved);
        opponent_start(saved);
    }
}
