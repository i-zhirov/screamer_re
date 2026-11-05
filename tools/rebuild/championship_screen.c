/* The rebuilt `championship_screen` module.
 *
 * The original sits at linear 0x2cf68 in STARTH.EXE and holds 370 instructions
 * and 1357 bytes up to the `ret` at 0x2d4b4.  The record keeps the 1360-byte
 * extent and sets no function size.
 *
 * The only caller at 0x295f1 is the state dispatcher `race_state_update` at
 * 0x29564, case 5.  That dispatcher is a `race_frame` callee at 0x2807c, so the
 * module takes the section `race_frame callees`.
 *
 * The selected record `DAT_001203dc` picks one 0x2c-byte record from the table
 * at object-3 offset 0x22bc.  The table holds the background names `champ0`
 * through `champ7` at +0x18; the loader at 0x2c7a8 reads `champ<N>H.pcx` for
 * this screen.  The text list `DAT_00120030` is the `champ` list that
 * `menu_space_init` loads at 0x29d08.
 *
 * When the record type is 3, the function draws the championship standings: the
 * two rectangles, the title, the four column headers, and the six rows.  Each
 * row holds the position, the car number `DAT_00120400[i]`, the race time
 * `DAT_001203e8[i]` as minutes.seconds.hundredths, and the points
 * `DAT_00120418[i]`.  The function sums the points into `DAT_001203e4` and
 * draws the two summary lines.
 *
 * When the record type is not 3, the function draws the centred list of the
 * `champ` group that the record selects, and it calls `net_race_screen` when the
 * frame timer `DAT_0011d24c` exceeds 10.0.  The footer panel appears only when
 * the low seven bits of the game mode `DAT_000007fd` are below 0x48.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 * The original converts the time with the truncating helper at 0x4d9f4, so the
 * local FPU stub makes `__CHP` truncate toward zero.  The constants 100.0 and
 * 10.0 sit at object-3 offsets 0x124c and 0x1250, so the module reads them from
 * the externs `DAT_0010124c` and `DAT_00101250`, because the harness supplies
 * them.
 */
extern unsigned int DAT_000007fd;
extern char *DAT_00120030[];
extern int DAT_001201c0[];
extern int DAT_001203dc;
extern int DAT_001203e4;
extern float DAT_001203e8[];
extern int DAT_00120400[];
extern int DAT_00120418[];
extern int DAT_00120804;
extern float DAT_0011d24c;
extern float DAT_0010124c;
extern float DAT_00101250;
extern char *DAT_0011fc60;

struct champ_record {
    int type;
    int value;
    int title_index;
    int group;
    int field_10;
    int field_14;
    char name[12];
    int field_24;
    int font;
};

extern struct champ_record DAT_001022bc[];

extern void video_rect_copy(int x, int y, int width, int height);
extern void panel_draw_centered(char *text, int y, int font, int color);
extern void text_draw(int x, int y, char *text, int font, int color);
extern void text_measure(int *width, int *height, char *text, int font);
extern int sprintf(char *buffer, const char *format, ...);
extern void net_race_screen(void);

void championship_screen(void)
{
    char buffer[0x80];
    struct champ_record *record;
    int width;
    int height;
    int x;
    int y;
    int i;
    int start;
    int font;
    int whole;
    int hundredths;
    int minutes;
    int seconds;

    DAT_001203e4 = 0;
    record = &DAT_001022bc[DAT_001203dc];
    if (record->type == 3) {
        video_rect_copy(0x20, 0x48, 0x240, 0x110);
        video_rect_copy(0x20, 0x48, 0x240, 0x110);
        panel_draw_centered(
            (char *)DAT_00120030[DAT_001201c0[6 + record->title_index]],
            2, 2, 2);
        text_draw(0x38, 0x60,
            (char *)DAT_00120030[DAT_001201c0[1]], 0, 0xf);
        text_draw(0xb0, 0x60,
            (char *)DAT_00120030[DAT_001201c0[0]], 0, 0xf);
        text_draw(0x154, 0x60,
            (char *)DAT_00120030[DAT_001201c0[2]], 0, 0xf);
        text_draw(0x1ec, 0x60,
            (char *)DAT_00120030[DAT_001201c0[3]], 0, 0xf);
        y = 0x3e;
        for (i = 0; i < 6; i = i + 1) {
            sprintf(buffer, "-%d-", i + 1);
            text_draw(0x4c, y + y, buffer, 0, 0xf);
            if (DAT_00120400[i] == -1) {
                text_draw(0xb0, y + y, "\x01-", 0, 0xf);
                text_draw(0x118, y + y, "\x01--.--.--", 0, 0xf);
                text_draw(0x214, y + y, "\x01-", 0, 0xf);
            } else if (DAT_00120804 == 1 && record->field_24 == i) {
                text_draw(0xb0, y + y, "\x0110", 0, 0xf);
                text_draw(0x118, y + y, "\x0100.00.00", 0, 0xf);
                text_draw(0x214, y + y, "\x010", 0, 0xf);
            } else {
                sprintf(buffer, "\x01%d", DAT_00120400[i]);
                text_draw(0xb0, y + y, buffer, 0, 0xf);
                whole = (int)DAT_001203e8[i];
                hundredths = (int)((DAT_001203e8[i] - (float)whole)
                    * DAT_0010124c);
                minutes = (int)DAT_001203e8[i] / 60;
                seconds = (int)DAT_001203e8[i] % 60;
                sprintf(buffer, "\x01%02d.%02d.%02d",
                    minutes, seconds, hundredths);
                text_draw(0x118, y + y, buffer, 0, 0xf);
                DAT_001203e4 = DAT_001203e4 + DAT_00120418[i];
                sprintf(buffer, "\x01%d", DAT_00120418[i]);
                text_draw(0x214, y + y, buffer, 0, 0xf);
            }
            y = y + 0xe;
        }
        y = y + 4;
        sprintf(buffer, "%s:\x01%02d",
            (char *)DAT_00120030[DAT_001201c0[5]], DAT_001203e4);
        text_measure(&width, &height, buffer, 0);
        width = width / 2;
        x = (0x114 - width) / 2;
        text_draw(x + x, y + y, buffer, 0, 0xf);
        y = y + 0x18;
        sprintf(buffer, "%s:%d",
            (char *)DAT_00120030[DAT_001201c0[4]], record->value);
        text_measure(&width, &height, buffer, 0);
        width = width / 2;
        x = 0x140 - width;
        text_draw(x, y + y, buffer, 0, 0xf);
    } else {
        start = DAT_001201c0[record->group];
        y = 0x20;
        for (;;) {
            i = start + 1;
            if (i >= DAT_001201c0[record->group + 1]) {
                break;
            }
            font = record->font;
            text_measure(&width, &height,
                (char *)DAT_00120030[start], font);
            width = width / 2;
            x = (0x140 - width) / 2;
            text_draw(x + x, y + y,
                (char *)DAT_00120030[start], font, 2);
            start = start + 1;
            height = height / 2;
            y = y + height;
        }
        if (DAT_0011d24c > DAT_00101250) {
            net_race_screen();
        }
    }
    if ((DAT_000007fd & 0x7f) < 0x48) {
        panel_draw_centered(DAT_0011fc60, 0xbe, 0, 2);
    }
}
