/* The rebuilt `opponent_effect_update` module.
 *
 * The original sits at linear 0x15b80 in STARTH.EXE and holds 14 instructions
 * and 41 bytes up to the `ret` at 0x15ba8.  The next function starts at
 * 0x15bac, so the store record keeps the 44-byte extent and sets no separate
 * function size.  The identity check uses 14 instructions.
 *
 * The three view groups call this function on the player object at object-3
 * offset 0xf91c: replay_view_update at 0x202c0 calls it at 0x2033f,
 * cng_view_update at 0x20360 calls it at 0x203ac, and sla_view_update at
 * 0x203c4 calls it at 0x20410.  The sibling car group at 0x15b08 and traffic
 * group at 0x15b44 have the same shape, with the nearby marker and the
 * proximity pass added.  The function is the opponent effect group of the
 * view update.
 *
 * The object pointer arrives in `eax`.  The function first places the player
 * body points through 0x17c6c.  It returns at once when the state field
 * `+0x148` is not zero.  Otherwise it starts the nearby opponents at 0x124c0
 * with the same player object.
 *
 * The three view callers, the opponent start step, and the state gate support
 * the name `opponent_effect_update`.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
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
extern void opponent_start(void *object);

void opponent_effect_update(void *object)
{
    void *volatile saved = object;

    player_body_point_place(saved);
    if (*(int *)((char *)saved + 0x148) == 0) {
        opponent_start(saved);
    }
}
