/* The rebuilt `race_results_screen` module.
 *
 * The original sits at linear 0x2a558 in STARTH.EXE and holds 343 instructions
 * and 1323 bytes up to the `ret` at 0x2aa82.  The record keeps the 1324-byte
 * extent and sets no function size.
 *
 * The only caller is the race-menu handler at 0x2a335.  The named ancestors are
 * `net_race_screen` at 0x28ba4 and `net_race_lobby` at 0x2d7c4, so the module
 * takes the section `race_frame callees`.
 *
 * The function first shows the track loading screen.  It reads the record of
 * the chosen player at object-3 offset 0x2099c, clears the video buffer, loads
 * `load<N>H.pcx` from `gfx\graf.dat`, sets the palette, builds the `map<N>`
 * name, and publishes the race fields.  It then runs the cleanup, the race
 * session, and the menu init.
 *
 * The input word `DAT_00118e1c` gates the results.  When the bits 0x1020 are
 * clear, the function loads `champ5h.pcx`, marks the local result as finished,
 * collects the finished results, sorts them by the race time, and draws the
 * lines `N. <name> <time> <car>` at x 0x40.  It then waits for the two input
 * flags or the two local gates, and it runs the frame step and the input poll.
 * It restores the saved screen flags and runs the menu frame at the end.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 * The original converts the time with the truncating helper at 0x4d9f4, so the
 * local FPU stub makes `__CHP` truncate toward zero.
 * The float constant 100.0 sits at object-3 offset 0x108b, so the module reads
 * it from the extern `DAT_0010108b`, because the harness does not load the
 * constant pool.
 */
extern unsigned char DAT_00000003;
extern unsigned char DAT_0000001e;
extern unsigned char DAT_00113cd8[];
extern char DAT_0011d550[];
extern char DAT_00120430[];
extern char DAT_0011d294[];
extern char DAT_0010cf5c[];
extern char *DAT_0011fc3c;
extern char *DAT_0011fc48;
extern int DAT_001021d8[];
extern int DAT_00118e1c;
extern unsigned int DAT_0011d274;
extern int DAT_0011d250;
extern int DAT_0011d254;
extern int DAT_0011d2a0;
extern int DAT_0011d2b0;
extern int DAT_0011d2b4;
extern int DAT_0011d2b8;
extern int DAT_0011d2c0;
extern int DAT_0011d2c8;
extern int DAT_0011d2cc;
extern int DAT_001203bc;
extern int DAT_001203c4;
extern int DAT_00120b5c;
extern float DAT_0010108b;
extern float DAT_0011d268;
extern float DAT_0011d26c;
extern void *DAT_00102dec;
extern void *DAT_0011fad0;
extern void *DAT_00121528;

struct net_player {
    int field_00;
    int field_04;
    int field_08;
    char name[4];
    int field_10;
    int field_14;
    int field_18;
    int field_1c;
    int field_20;
    int field_24;
    int field_28;
    int field_2c;
    int field_30;
    int field_34;
};

extern struct net_player DAT_0012099c[];

extern void *memset(void *target, unsigned int count, int value);
extern void *memcpy(void *target, const void *source, unsigned int count);
extern int pcx_load(char *file, char *member, void *pixels, char *palette);
extern void palette_set(char *palette);
extern void message_show(char *buffer);
extern int sprintf(char *buffer, const char *format, ...);
extern void video_fill(void);
extern void video_block_swap_a(void);
extern void video_block_swap_b(void);
extern void panel_draw_centered(char *text, int y, int font, int color);
extern void text_draw(int x, int y, char *text, int font, int color);
extern void race_cleanup(void);
extern int sub_1fd70(void);
extern void music_step(void);
extern void menu_space_init(void);
extern void frame_step(void);
extern void input_poll(void);
extern void menu_frame(void);
extern void state_cells_set(int value);
extern void sub_15150(void);
extern int race_count(void);
extern int race_local_index(void);

void race_results_screen(void)
{
    char name[0x300];
    char time_text[0x20];
    int order[8];
    float times[8];
    struct net_player *self;
    unsigned char *entry;
    unsigned int saved;
    void *context;
    int index;
    int i;
    int j;
    int finish;
    int whole;
    int hundredths;
    int minutes;
    int seconds;
    int y;

    memset(DAT_00121528, 0x3e800, 0);
    video_fill();
    saved = DAT_0011d274;
    self = &DAT_0012099c[DAT_00120b5c];
    DAT_0011d274 = self->field_20;
    if (self->field_14 > 5) {
        DAT_0011d274 = DAT_0011d274 | 0x08;
        self->field_14 = self->field_14 - 6;
    }
    DAT_001203bc = self->field_14;
    sprintf(name, "load%d%c.pcx", self->field_14 + 1, 0x48);
    if (pcx_load("gfx\\graf.dat", name, DAT_0011fad0, DAT_0010cf5c) != 0) {
        sub_15150();
    }
    palette_set(DAT_0010cf5c);
    message_show(DAT_0010cf5c);
    sprintf(DAT_0011d294, "map%d", self->field_14 + 1);
    DAT_0011d2a0 = DAT_001203c4;
    DAT_0011d2b4 = self->field_1c;
    DAT_0011d2b0 = self->field_18;
    DAT_0011d26c = *(float *)(DAT_00120430 + self->field_14 * 0x50);
    context = DAT_00102dec;
    DAT_0011d2c8 = *(int *)((char *)context + 0x38);
    DAT_0011d2cc = *(int *)((char *)context + 0x48);
    memcpy(DAT_00121528, DAT_0011fad0, 0x3e800);
    if ((DAT_0011d274 & 0x10) != 0) {
        video_block_swap_a();
    }
    if ((DAT_0011d274 & 0x20) != 0) {
        video_block_swap_b();
    }
    panel_draw_centered(DAT_0011fc3c, 0x50, 3, 0);
    video_fill();
    DAT_0011d254 = race_count();
    DAT_0011d250 = race_local_index();
    state_cells_set(DAT_001021d8[*(int *)((char *)context + 8)]);
    DAT_0011d2b8 = *(int *)((char *)context + 8);
    race_cleanup();
    sub_1fd70();
    music_step();
    menu_space_init();
    if ((DAT_00118e1c & 0x1020) != 0) {
        menu_frame();
        DAT_0011d274 = saved;
        return;
    }
    if (pcx_load("gfx\\graf.dat", "champ5h.pcx", DAT_0011fad0,
                 DAT_0010cf5c) != 0) {
        sub_15150();
    }
    memcpy(DAT_00121528, DAT_0011fad0, 0x3e800);
    if ((DAT_0011d274 & 0x10) != 0) {
        video_block_swap_a();
    }
    if ((DAT_0011d274 & 0x20) != 0) {
        video_block_swap_b();
    }
    panel_draw_centered(DAT_0011fc48, 2, 3, 2);
    entry = DAT_00113cd8 + DAT_0011d250 * 680;
    *(int *)entry = -3;
    *(float *)(entry + 0x2c) = DAT_0011d268;
    finish = 0;
    for (i = 0; i < DAT_0011d254; i++) {
        entry = DAT_00113cd8 + i * 680;
        if (*(int *)entry == -3) {
            order[finish] = i;
            times[finish] = *(float *)(entry + 0x2c);
            finish = finish + 1;
        }
    }
    if (finish > 1) {
        for (i = 0; i < finish; i++) {
            for (j = 0; j < finish - i - 1; j++) {
                if (times[j] >= times[j + 1]) {
                    int swap_order = order[j];
                    float swap_time = times[j];

                    order[j] = order[j + 1];
                    order[j + 1] = swap_order;
                    times[j] = times[j + 1];
                    times[j + 1] = swap_time;
                }
            }
        }
    }
    y = 0x40;
    for (i = 0; i < finish; i++) {
        whole = (int)times[i];
        hundredths = (int)((times[i] - (float)whole) * DAT_0010108b);
        minutes = whole / 60;
        seconds = whole % 60;
        sprintf(time_text, "%02d.%02d.%02d", minutes, seconds, hundredths);
        index = order[i];
        sprintf(name, "\x01%d. %s %s \x02%s", i + 1,
                DAT_0012099c[index].name, time_text,
                DAT_0011d550 + DAT_0012099c[index].field_10 * 0x44 + 0x14);
        text_draw(0x40, y + y, name, 0, 2);
        y = y + 0x10;
    }
    video_fill();
    palette_set(DAT_0010cf5c);
    for (;;) {
        frame_step();
        if (DAT_00000003 != 0) {
            break;
        }
        if (DAT_0000001e != 0) {
            break;
        }
        if (DAT_0011d2c0 == 0) {
            continue;
        }
        if (DAT_0011d2cc == 0) {
            continue;
        }
        input_poll();
    }
    menu_frame();
    DAT_0011d274 = saved;
}
