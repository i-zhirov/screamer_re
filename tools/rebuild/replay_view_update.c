/* The rebuilt `replay_view_update` module.
 *
 * The original sits at linear 0x202c0 in STARTH.EXE and holds 37 instructions
 * and 160 bytes up to the `ret` at 0x2035f.  The next recorded function starts
 * at 0x20360, so the store record keeps the 160-byte extent and sets no
 * separate function size.  The identity check uses 37 instructions.
 *
 * The only direct caller is the race frame update at 0x1ff70.  That function
 * selects this view group when bit 4 of the mode byte `DAT_0011d260` is set.
 * The mode 4 handler at 0x12e18 starts the ATTACK_RES replay session, so the
 * group belongs to the replay race.  The record takes the section
 * `Program functions`, because the frame update calls it directly.
 *
 * The group appends the current player view keyframe to the ghost record that
 * the index `DAT_00118ed4` selects, then updates the playback block
 * `DAT_00118ed8` and applies it to the display object.  It then runs the
 * player view, physics, effect, step, and opponent groups, the player update,
 * and the player car link on the fixed player object 0xf91c.
 *
 * The two gates require input flag `DAT_00118e1d` bit 0 and mode byte
 * `DAT_0011d260` bit 4.  The mode 4 handler and the keyframe append at 0x231f0
 * support the name `replay_view_update`.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses no floating-point instruction and links no FPU stub.
 */
extern unsigned char DAT_00118e1d;
extern unsigned char DAT_0011d260;
extern int DAT_00118ed4;
extern unsigned char DAT_00118e8c[];
extern unsigned char DAT_00118ed8[];

extern void race_position_update(void);
extern void player_car_view_update(void *player);
extern void path_keyframe_append(void *player, void *record);
extern void path_playback_update(void *block, int sample);
extern void player_physics_update(void *player);
extern void player_effect_update(void *player);
extern void player_step_update(void *player);
extern void player_opponent_update(void *player);
extern void player_update(void);
extern void player_car_link(void *player);
extern void path_playback_apply(void *block);

void replay_view_update(void)
{
    void *record;

    if ((DAT_00118e1d & 1) == 0) {
        return;
    }
    if ((DAT_0011d260 & 4) == 0) {
        return;
    }
    race_position_update();
    player_car_view_update((void *)0xf91c);
    record = DAT_00118e8c + DAT_00118ed4 * 0x24;
    path_keyframe_append((void *)0xf91c, record);
    path_playback_update(DAT_00118ed8, 0);
    player_physics_update((void *)0xf91c);
    player_effect_update((void *)0xf91c);
    player_step_update((void *)0xf91c);
    player_opponent_update((void *)0xf91c);
    player_update();
    player_car_link((void *)0xf91c);
    path_playback_apply(DAT_00118ed8);
}
