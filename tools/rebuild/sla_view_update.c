/* The rebuilt `sla_view_update` module.
 *
 * The original sits at linear 0x203c4 in STARTH.EXE and holds 23 instructions
 * and 98 bytes up to the `ret` at 0x20425.  Two padding bytes at 0x20426
 * precede the next function at 0x20428, so the store record keeps the 100-byte
 * extent and sets no separate function size.  The identity check uses 23
 * instructions.
 *
 * The only direct caller is the race frame update at 0x1ff70, at the call site
 * 0x1ffcd.  The frame update selects this view group when bit 0x40 of the mode
 * byte `DAT_0011d260` is set, and the loader fills the SLA marker table only
 * for that mode.  The record takes the section `Program functions`, because
 * the frame update calls the function directly.
 *
 * The two gates require input flag `DAT_00118e1d` bit 0 and mode byte
 * `DAT_0011d260` bit 0x40.  The group then checks the SLA markers on the fixed
 * player object 0xf91c through `sla_marker_check` at 0x18ba8 and runs the
 * player view, physics, effect, step, and opponent groups, the player update,
 * and the player car link on the same object.
 *
 * The SLA marker check, the mode 0x40 gate, and the player object support the
 * name `sla_view_update`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses no floating-point instruction and links no FPU stub.
 */
extern unsigned char DAT_00118e1d;
extern unsigned char DAT_0011d260;

extern void sla_marker_check(void *player);
extern void player_car_view_update(void *player);
extern void player_physics_update(void *player);
extern void player_effect_update(void *player);
extern void player_step_update(void *player);
extern void player_opponent_update(void *player);
extern void player_update(void);
extern void player_car_link(void *player);

void sla_view_update(void)
{
    if ((DAT_00118e1d & 1) == 0) {
        return;
    }
    if ((DAT_0011d260 & 0x40) == 0) {
        return;
    }
    sla_marker_check((void *)0xf91c);
    player_car_view_update((void *)0xf91c);
    player_physics_update((void *)0xf91c);
    player_effect_update((void *)0xf91c);
    player_step_update((void *)0xf91c);
    player_opponent_update((void *)0xf91c);
    player_update();
    player_car_link((void *)0xf91c);
}
