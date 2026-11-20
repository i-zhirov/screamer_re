/* The rebuilt `cng_view_update` module.
 *
 * The original sits at linear 0x20360 in STARTH.EXE and holds 23 instructions
 * and 98 bytes up to the `ret` at 0x203c1.  Two padding bytes at 0x203c2
 * precede the next function at 0x203c4, so the store record keeps the 100-byte
 * extent and sets no separate function size.  The identity check uses 23
 * instructions.
 *
 * The only direct caller is the race frame update at 0x1ff70, at the call site
 * 0x1ffc8.  The frame update selects this view group when bit 0x20 of the mode
 * byte `DAT_0011d260` is set.  The game mode 0x20 is the CNG mode: the game
 * mode start at 0x13120 clears the opponent records, and the spline loader at
 * 0x14fb8 builds the `CNG` member for that mode.  The frame update calls the
 * function directly, so the record takes the section `Program functions`.
 *
 * The two gates require input flag `DAT_00118e1d` bit 0 and mode byte
 * `DAT_0011d260` bit 0x20.  The group then updates the opponent score on the
 * fixed player object 0xf91c through the helper at 0x18b60.  That helper adds
 * 1.0 to the score float at +0x790 when the started opponent counter at +0x7b8
 * rises above the stored count at +0x7bc, and `opponent_start` at 0x124c0
 * increments the same counter.
 *
 * The group then runs the player view, physics, effect, step, and opponent
 * groups, the player update, and the player car link on the same object.  The
 * sequence is the same as `sla_view_update` at 0x203c4, but the mode gate and
 * the first callee differ.
 *
 * The CNG mode gate, the opponent score helper, and the player object support
 * the name `cng_view_update`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has the EBP frame.  The C form with `-of+` adds `mov esp,ebp`
 * and reorders the calls, so the recipe uses the exact
 * `cng_view_update.asm` body.  This C source stays as the readable behavioral
 * reference.  The function uses no floating-point instruction and links no FPU
 * stub.
 */
extern unsigned char DAT_00118e1d;
extern unsigned char DAT_0011d260;

extern void opponent_score_update(void *player);
extern void player_car_view_update(void *player);
extern void player_physics_update(void *player);
extern void player_car_effect_update(void *player);
extern void player_step_update(void *player);
extern void player_opponent_update(void *player);
extern void player_update(void);
extern void player_car_link(void *player);

void cng_view_update(void)
{
    if ((DAT_00118e1d & 1) == 0) {
        return;
    }
    if ((DAT_0011d260 & 0x20) == 0) {
        return;
    }
    opponent_score_update((void *)0xf91c);
    player_car_view_update((void *)0xf91c);
    player_physics_update((void *)0xf91c);
    player_car_effect_update((void *)0xf91c);
    player_step_update((void *)0xf91c);
    player_opponent_update((void *)0xf91c);
    player_update();
    player_car_link((void *)0xf91c);
}
