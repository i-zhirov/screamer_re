/* The rebuilt `race_frame_update` module.
 *
 * The original sits at linear 0x1ff70 in STARTH.EXE and holds 47 instructions
 * and 204 bytes up to the `ret` at 0x2003b.  The next function starts at
 * 0x2003c, so the store record keeps the 204-byte extent.  The identity check
 * uses 47 instructions.
 *
 * The only direct caller at 0x1ff54 is the frame loop at 0x1ff14.  The race
 * session driver at 0x1fd70 calls that loop at 0x1fdb5 and 0x1fe7a.  The
 * neighbouring records call this function the frame update at 0x1ff70.  The
 * caller chain and the five view groups that it runs support the name
 * `race_frame_update`.
 *
 * The function first reads the input byte DAT_00118e1c bit 0x20 and sets the
 * exit state DAT_00118e18 to 1.  It then reads the input byte DAT_00118e1d:
 * bit 4 sets the exit state to -1, and bit 2 calls the wait helper at 0x1f8dc
 * and clears DAT_00118e6c.  A non-zero exit state returns at once.
 *
 * The frame then runs in order: `timing_update`, the view groups at 0x202c0,
 * 0x20360, 0x203c4, 0x2003c, and 0x201d0, `ai_update`, `opponent_update` with
 * 0, `camera_update` with object 0x109a0, and `sound_effects_update` with the
 * driver 0x6350.
 *
 * The two video swaps at 0x1075f and 0x107a1 run only when the byte
 * DAT_0011d274 holds bit 0x10 or bit 0x20.  The frame then runs
 * `race_hud_draw`, `race_intro_update`, the handler at 0x24880,
 * `race_finish_laps_update`, the handler at 0x20f98, and `race_finish_update`.
 *
 * The function uses no floating-point instruction and no assembly.  The
 * original has the EBP frame, so the module builds with `-of+`.
 */
extern unsigned char DAT_00118e1c;
extern unsigned char DAT_00118e1d;
extern int DAT_00118e18;
extern int DAT_00118e6c;
extern unsigned char DAT_0011d274;

extern void func_1f8dc(void);
extern void timing_update(void);
extern void func_202c0(void);
extern void func_20360(void);
extern void func_203c4(void);
extern void race_simulation_update(void);
extern void traffic_simulation_update(void);
extern void ai_update(void);
extern void opponent_update(int value);
extern void camera_update(void *object);
extern void sound_effects_update(int driver);
extern void video_block_swap_a(void);
extern void video_block_swap_b(void);
extern void race_hud_draw(void);
extern void race_intro_update(void);
extern void func_24880(void);
extern void race_finish_laps_update(void);
extern void func_20f98(void);
extern void race_finish_update(void);

void race_frame_update(void)
{
    if ((DAT_00118e1c & 0x20) != 0) {
        DAT_00118e18 = 1;
    }
    if ((DAT_00118e1d & 4) != 0) {
        DAT_00118e18 = -1;
    }
    if ((DAT_00118e1d & 2) != 0) {
        func_1f8dc();
        DAT_00118e6c = 0;
    }
    if (DAT_00118e18 != 0) {
        return;
    }

    timing_update();
    func_202c0();
    func_20360();
    func_203c4();
    race_simulation_update();
    traffic_simulation_update();
    ai_update();
    opponent_update(0);
    camera_update((void *)0x109a0);
    sound_effects_update(0x6350);

    if ((DAT_0011d274 & 0x10) != 0) {
        video_block_swap_a();
    }
    if ((DAT_0011d274 & 0x20) != 0) {
        video_block_swap_b();
    }

    race_hud_draw();
    race_intro_update();
    func_24880();
    race_finish_laps_update();
    func_20f98();
    race_finish_update();
}
