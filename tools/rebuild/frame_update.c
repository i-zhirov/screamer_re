/* The rebuilt `frame_update` module.
 *
 * The original sits at linear 0x27f94 in STARTH.EXE and holds 63 instructions
 * and 212 bytes: the five register saves, the frame, the 0x8-byte local area,
 * the clear of the 20 key-state dwords at `DAT_0001fb08`, the reset of
 * `DAT_00020988` and the frame value `DAT_0011d24c`, the memory probe at
 * `0x2f8ca` with the list pointer `DAT_000062ac`, the menu init at `0x29d08`,
 * the menu frame at `0x29f80`, the 6-frame counter `DAT_000207f8`, the frame
 * loop with `frame_step` at `0x2aa84`, `race_frame` at `0x2807c`, the input
 * poll at `0x26448`, the music call at `0x2ff14` with the record
 * `DAT_000062b0`, and `video_fill` at `0x2aa98`, and the exit path with the
 * `memset` at `0x8c160`, the second `video_fill`, and `race_cleanup` at
 * `0x2a1b0`.
 *
 * The frame loop runs while the frame flag `DAT_0011d234` is 0.  The input
 * poll runs only when the race context `DAT_00002dec` holds a non-zero word at
 * `+0x48` and the two input flags `DAT_0011d2c0` and `DAT_0011d2cc` are
 * non-zero.  The music call runs for the same context condition, after the
 * optional input poll.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern void *DAT_000062ac;
extern char DAT_000062b0[];
extern void *DAT_00002dec;
extern int DAT_0001fb08[20];
extern int DAT_00020988;
extern int DAT_00020880;
extern int DAT_000207f8;
extern int DAT_0011d2c0;
extern int DAT_0011d2cc;
extern int DAT_0011d234;
extern float DAT_0011d24c;
extern void *DAT_00121528;

extern unsigned int memory_available(void *pool);
extern void menu_space_init(void);
extern void menu_frame(void);
extern void frame_step(void);
extern void race_frame(void);
extern int input_poll(void);
extern int music_play(char *record);
extern void video_fill(void);
extern void race_cleanup(void);
extern void *memset(void *dest, unsigned int count, int value);

void frame_update(void)
{
    int index;

    for (index = 0; index < 20; ++index) {
        DAT_0001fb08[index] = 0;
    }
    DAT_00020988 = 0;
    DAT_0011d24c = 0.0f;
    DAT_00020880 = (int)memory_available(DAT_000062ac);
    menu_space_init();
    menu_frame();
    DAT_000207f8 = DAT_000207f8 + 1;
    if (DAT_000207f8 > 5) {
        DAT_000207f8 = 0;
    }
    do {
        frame_step();
        race_frame();
        if (*(int *)((char *)DAT_00002dec + 0x48) != 0) {
            if (DAT_0011d2c0 != 0 && DAT_0011d2cc != 0) {
                input_poll();
            }
            music_play(DAT_000062b0);
        }
        video_fill();
    } while (DAT_0011d234 == 0);
    memset(DAT_00121528, 0x3e800, 0);
    video_fill();
    race_cleanup();
}
