/* The rebuilt `object_update_all` module.
 *
 * The original sits at linear 0x2786c in STARTH.EXE and holds 84 instructions
 * and 309 bytes up to the `ret` at 0x279a0: the six register saves, the frame,
 * the five record passes, the seven subsystem calls, and the epilogue.
 *
 * Every pass walks the `DAT_0010f8ec` records at `DAT_00110ab8` with the
 * 0x590-byte stride and the shared loop counter `DAT_0010634c`.  The five
 * passes call `object_transform` at 0x1cd44, `object_player_input` at 0x1c760,
 * `object_physics` at 0x1c810, `object_prepare` at 0x1c79c, and
 * `object_car_link` at 0x1c84c.
 *
 * The seven calls then run the race timeout at 0x27f44, the player update at
 * 0x19ab0, the ai update at 0x205c4, the opponent update at 0x20954 with the
 * value 1, the camera update at 0x8ac78 with the record 0x109a0, the sound
 * effects update at 0x81ab0 with the driver 0x6350, and the hud update at
 * 0x27a34.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_0010f8ec;
extern int DAT_0010634c;
extern unsigned char DAT_00110ab8[];

extern void object_transform(void *object);
extern void object_player_input(void *object);
extern void object_physics(void *object);
extern void object_prepare(void *object);
extern void object_car_link(void *object);

extern void race_timeout(void);
extern void player_update(void);
extern void ai_update(void);
extern void opponent_update(int value);
extern void camera_update(void *camera);
extern void sound_effects_update(int driver);
extern void hud_update(void);

void object_update_all(void)
{
    for (DAT_0010634c = 0; DAT_0010634c < DAT_0010f8ec; DAT_0010634c++) {
        object_transform(DAT_00110ab8 + DAT_0010634c * 0x590);
    }
    for (DAT_0010634c = 0; DAT_0010634c < DAT_0010f8ec; DAT_0010634c++) {
        object_player_input(DAT_00110ab8 + DAT_0010634c * 0x590);
    }
    for (DAT_0010634c = 0; DAT_0010634c < DAT_0010f8ec; DAT_0010634c++) {
        object_physics(DAT_00110ab8 + DAT_0010634c * 0x590);
    }
    for (DAT_0010634c = 0; DAT_0010634c < DAT_0010f8ec; DAT_0010634c++) {
        object_prepare(DAT_00110ab8 + DAT_0010634c * 0x590);
    }
    for (DAT_0010634c = 0; DAT_0010634c < DAT_0010f8ec; DAT_0010634c++) {
        object_car_link(DAT_00110ab8 + DAT_0010634c * 0x590);
    }
    race_timeout();
    player_update();
    ai_update();
    opponent_update(1);
    camera_update((void *)0x109a0);
    sound_effects_update(0x6350);
    hud_update();
}
