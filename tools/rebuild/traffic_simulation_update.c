/* The rebuilt `traffic_simulation_update` module.
 *
 * The original sits at linear 0x201d0 in STARTH.EXE and holds 66 instructions
 * and 237 bytes up to the `ret` at 0x202bc.  The next function starts at
 * 0x202c0, so the store record keeps the 240-byte extent and sets no separate
 * function size.  The identity check uses 66 instructions.
 *
 * The function is the traffic view group.  The frame update at 0x1ff70 calls
 * it when bit 0x10 of the mode byte `DAT_0011d260` is set, and bit 0 of
 * `DAT_00118e1d` is the input gate.  The group runs the player car view, the
 * player physics, the player effect, and the player step groups on the player
 * object at object-3 offset 0xf91c.  It then runs `traffic_step` with the retry
 * limit 3.
 *
 * The first loop walks the 0x2a8-byte traffic records at `DAT_00113cd8` while
 * the index stays below the count `DAT_00115218`, and it runs the record
 * update at 0x1f108 on each record.  The group then runs the race position
 * update at 0x20428, the traffic effect group at 0x15b44, `player_update` at
 * 0x19ab0, and `player_car_link` at 0x15c1c.  The second loop walks the same
 * records and runs `traffic_object_link` at 0x1ef68 on each record.
 *
 * The shared counter is `DAT_0000634c`.
 *
 * The frame update caller, the two gates, the player steps, the traffic step,
 * and the two record loops support the name `traffic_simulation_update`.  The
 * original has no symbol, so the record keeps the name with the confidence
 * inferred.
 *
 * The function uses no floating-point instruction.  The original has the EBP
 * frame, so the module builds with `-of+`.
 */
extern unsigned char DAT_00118e1d;
extern unsigned char DAT_0011d260;
extern int DAT_00115218;
extern int DAT_0000634c;
extern unsigned char DAT_00113cd8[];

extern void player_car_view_update(void *player);
extern void player_physics_update(void *player);
extern void player_effect_update(void *player);
extern void player_step_update(void *player);
extern void traffic_step(int limit);
extern void traffic_record_update(void *record);
extern void race_position_update(void);
extern void traffic_effect_update(void *player);
extern void player_update(void);
extern void player_car_link(void *player);
extern void traffic_object_link(void *record);

void traffic_simulation_update(void)
{
    if ((DAT_00118e1d & 1) == 0) {
        return;
    }
    if ((DAT_0011d260 & 0x10) == 0) {
        return;
    }
    player_car_view_update((void *)0xf91c);
    player_physics_update((void *)0xf91c);
    player_effect_update((void *)0xf91c);
    player_step_update((void *)0xf91c);
    traffic_step(3);
    for (DAT_0000634c = 0; DAT_0000634c < DAT_00115218; DAT_0000634c++) {
        traffic_record_update(DAT_00113cd8 + DAT_0000634c * 0x2a8);
    }
    race_position_update();
    traffic_effect_update((void *)0xf91c);
    player_update();
    player_car_link((void *)0xf91c);
    for (DAT_0000634c = 0; DAT_0000634c < DAT_00115218; DAT_0000634c++) {
        traffic_object_link(DAT_00113cd8 + DAT_0000634c * 0x2a8);
    }
}
