/* The rebuilt `race_records_screen` module.
 *
 * The original sits at linear 0x2d4b8 in STARTH.EXE and holds 224 instructions
 * and 778 bytes up to the `ret` at 0x2d7c2.  The record keeps the 780-byte
 * extent and sets no function size.
 *
 * The only direct caller at 0x295fb is case 6 of the state dispatcher
 * `race_state_update` at 0x29564.  The dispatcher is a `race_frame` callee at
 * 0x2807c, so the module takes the section `race_frame callees`.
 *
 * The function clears the screen area twice, draws the title of the selected
 * track, and draws the two list headers.  The title comes from the six-entry
 * pointer table at `DAT_0011fbfc`, indexed by the selected track
 * `DAT_0012098c`.  The headers are the pointers `DAT_0011fc28` and
 * `DAT_0011fc2c`.
 *
 * The first list shows the five best race times of the selected track at
 * `DAT_00120610 + index * 0x50`.  Each 0x50-byte record holds five floats at
 * +0x00, five 4-byte names at +0x14, and five flags at +0x28.  A flag that is
 * not zero marks a saved replay, so the line adds the suffix ` R`.  The row
 * that matches the selected entry `DAT_00120990` uses the color 1, and the
 * other rows use the color 0.
 *
 * The second list shows the five best lap times of the selected track at
 * `DAT_00120430 + index * 0x50`.  The rows use the color 0.
 *
 * Each line formats the position, the name, and the time as
 * `%d. %3s %02d.%02d.%02d`.  The minutes and the seconds come from the
 * truncated time divided by 60, and the hundredths come from the fractional
 * part times the float 100.0 at `DAT_001012a2`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 * The original converts the time with the truncating helper at 0x4d9f4, so the
 * local FPU stub makes `__CHP` truncate toward zero.
 */
extern int DAT_0012098c;
extern int DAT_00120990;
extern char *DAT_0011fbfc[];
extern char *DAT_0011fc28;
extern char *DAT_0011fc2c;
extern float DAT_001012a2;
extern unsigned char DAT_00120430[];
extern unsigned char DAT_00120610[];

extern void video_rect_copy(int x, int y, int width, int height);
extern void panel_draw_centered(char *text, int y, int font, int color);
extern int sprintf(char *buffer, const char *format, ...);

void race_records_screen(void)
{
    char buffer[0x20];
    unsigned char *lap;
    unsigned char *race;
    int whole;
    int minutes;
    int seconds;
    int hundredths;
    int i;
    int y;

    video_rect_copy(0x60, 0x48, 0x1c4, 0x140);
    video_rect_copy(0x60, 0x48, 0x1c4, 0x140);
    panel_draw_centered(DAT_0011fbfc[DAT_0012098c], 2, 3, 0);
    panel_draw_centered(DAT_0011fc28, 0x28, 0, 0);
    panel_draw_centered(DAT_0011fc2c, 0x78, 0, 0);

    race = DAT_00120610 + DAT_0012098c * 0x50;
    y = 0x3c;
    for (i = 0; i < 5; i = i + 1) {
        whole = (int)*(float *)(race + i * 4);
        minutes = whole / 60;
        seconds = whole % 60;
        hundredths = (int)((*(float *)(race + i * 4) - (float)whole)
            * DAT_001012a2);
        if (*(int *)(race + 0x28 + i * 4) != 0) {
            sprintf(buffer, "\x01%d. %3s %02d.%02d.%02d R", i + 1,
                (char *)(race + 0x14 + i * 4), minutes, seconds, hundredths);
        } else {
            sprintf(buffer, "\x01%d. %3s %02d.%02d.%02d  ", i + 1,
                (char *)(race + 0x14 + i * 4), minutes, seconds, hundredths);
        }
        panel_draw_centered(buffer, y, 1, DAT_00120990 == i);
        y = y + 0xa;
    }

    lap = DAT_00120430 + DAT_0012098c * 0x50;
    y = 0x8c;
    for (i = 0; i < 5; i = i + 1) {
        whole = (int)*(float *)(lap + i * 4);
        minutes = whole / 60;
        seconds = whole % 60;
        hundredths = (int)((*(float *)(lap + i * 4) - (float)whole)
            * DAT_001012a2);
        sprintf(buffer, "\x01%d. %3s %02d.%02d.%02d  ", i + 1,
            (char *)(lap + 0x14 + i * 4), minutes, seconds, hundredths);
        panel_draw_centered(buffer, y, 1, 0);
        y = y + 0xa;
    }
}
