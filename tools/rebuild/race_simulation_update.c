/* The rebuilt `race_simulation_update` module.
 *
 * The original sits at linear 0x2003c in STARTH.EXE and holds 104 instructions
 * and 404 bytes up to the `ret` at 0x201cf.  The next function starts at
 * 0x201d0, so the store record keeps the 404-byte extent and sets no function
 * size.  The identity check uses 104 instructions.
 *
 * The function is the race view group.  The frame update at 0x1ff70 calls it
 * for the race modes, and the mode byte `DAT_0011d260` bits 0 and 1 select the
 * path.  The group runs `func_20428`, then interleaves each player step with a
 * pass over the 0x590-byte car records at `DAT_00110ab8`.
 *
 * The player steps are `player_car_view_update` at 0x16f34, the physics group
 * at 0x15a80, the effect group at 0x15bac, the step group at 0x15ab8, the
 * proximity group at 0x15b08, `player_update` at 0x19ab0, and
 * `player_car_link` at 0x15c1c.  The object passes are `object_transform` at
 * 0x1cd44, `object_player_input` at 0x1c760, `object_physics` at 0x1c810,
 * `object_prepare` at 0x1c79c, the collision step at 0x1c7d0, and
 * `object_car_link` at 0x1c84c.
 *
 * The two gates clear the path when `DAT_00118e1d` bit 0 is clear or when the
 * mode byte holds neither bit 0 nor bit 1.  The shared loop counter is
 * `DAT_0010634c`, and the record count is `DAT_0010f8ec`.
 *
 * The caller, the two gates, the interleaved player steps, and the car passes
 * support the name `race_simulation_update`.  The original has no symbol, so
 * the record keeps the name with the confidence inferred.
 *
 * The function uses no floating-point instruction.  The original has the EBP
 * frame, so the module builds with `-of+`.
 */
extern unsigned char DAT_00118e1d;
extern unsigned char DAT_0011d260;
extern int DAT_0010f8ec;
extern int DAT_0010634c;
extern unsigned char DAT_00110ab8[];

extern void func_20428(void);
extern void player_car_view_update(void *player);
extern void player_physics_update(void *player);
extern void player_effect_update(void *player);
extern void player_step_update(void *player);
extern void player_proximity_update(void *player);
extern void player_update(void);
extern void player_car_link(void *player);

extern void object_transform(void *object);
extern void object_player_input(void *object);
extern void object_physics(void *object);
extern void object_prepare(void *object);
extern void object_collision_step(void *object);
extern void object_car_link(void *object);

void race_simulation_update(void)
{
    if ((DAT_00118e1d & 1) == 0) {
        return;
    }
    if ((DAT_0011d260 & 3) == 0) {
        return;
    }
    func_20428();
    player_car_view_update((void *)0xf91c);
    for (DAT_0010634c = 0; DAT_0010634c < DAT_0010f8ec; DAT_0010634c++) {
        object_transform(DAT_00110ab8 + DAT_0010634c * 0x590);
    }
    player_physics_update((void *)0xf91c);
    for (DAT_0010634c = 0; DAT_0010634c < DAT_0010f8ec; DAT_0010634c++) {
        object_player_input(DAT_00110ab8 + DAT_0010634c * 0x590);
    }
    player_effect_update((void *)0xf91c);
    for (DAT_0010634c = 0; DAT_0010634c < DAT_0010f8ec; DAT_0010634c++) {
        object_physics(DAT_00110ab8 + DAT_0010634c * 0x590);
    }
    player_step_update((void *)0xf91c);
    for (DAT_0010634c = 0; DAT_0010634c < DAT_0010f8ec; DAT_0010634c++) {
        object_prepare(DAT_00110ab8 + DAT_0010634c * 0x590);
    }
    player_proximity_update((void *)0xf91c);
    for (DAT_0010634c = 0; DAT_0010634c < DAT_0010f8ec; DAT_0010634c++) {
        object_collision_step(DAT_00110ab8 + DAT_0010634c * 0x590);
    }
    player_update();
    player_car_link((void *)0xf91c);
    for (DAT_0010634c = 0; DAT_0010634c < DAT_0010f8ec; DAT_0010634c++) {
        object_car_link(DAT_00110ab8 + DAT_0010634c * 0x590);
    }
}
