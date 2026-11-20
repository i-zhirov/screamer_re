/* The rebuilt `player_previous_speed_set` module.
 *
 * The original sits at linear 0x15bfc in STARTH.EXE and holds 11 instructions
 * and 31 bytes up to the `ret` at 0x15c1a.  The next function starts at
 * 0x15c1c after one padding byte at 0x15c1b, so the store record keeps the
 * 32-byte extent and sets no separate function size.  The identity check uses
 * 11 instructions.
 *
 * The only direct caller is player_car_view_update at 0x16f34, which calls
 * this function at 0x16fda on the player object at object-3 offset 0xf91c.
 * The caller sits in the section The vehicle state chain, so the record takes
 * the same section.
 *
 * The function reads the road speed `+0x50c` and writes the previous speed
 * `+0x588`.  player_car_suspension_update at 0x168b4 forms the speed change
 * from the road speed `+0x50c` minus the previous speed `+0x588`, so this
 * function latches the current speed for the next suspension step.  The
 * speed source, the previous-speed target, and the suspension consumer
 * support the name `player_previous_speed_set`.  The original has no symbol,
 * so the record keeps the name with the confidence inferred.
 *
 * The original stores the argument to the frame and reloads it before each
 * access (`sub esp,4`, `mov [ebp-4],eax`, `mov eax,[ebp-4]`).  A plain
 * parameter stays in a preserved register and produces no load.  The volatile
 * local reproduces the store and the reload, as `error_report.c` records for
 * the same frame-local pattern.
 *
 * The original has the EBP frame and holds two floating-point instructions,
 * `fld` and `fstp`.  The module builds with `-of+` and needs no math call.  The
 * harness links the FPU stub for the run-time symbols.  The module uses no
 * assembly implementation.
 */
void player_previous_speed_set(void *object)
{
    void *volatile saved = object;

    *(float *)((char *)saved + 0x588) = *(float *)((char *)saved + 0x50c);
}
