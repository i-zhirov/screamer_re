/* The rebuilt `loading_screen` module.
 *
 * The original sits at linear 0x2a364 in STARTH.EXE and holds 127 instructions
 * and 498 bytes: the four register saves, the frame with the 0x320-byte local
 * name buffer, the resource call at 0x2f326, the `load%d%c.pcx` build at
 * 0x4d32c, the `pcx_load` call at 0x1b5a4, the palette call at 0x81a05, the
 * message call at 0x258d0, the `memcpy` call at 0x8c150, the two swap calls at
 * 0x1075f and 0x107a1, the centered panel call at 0x2e7cc, the `map%d` build at
 * 0x4d32c, the `race_cleanup` call at 0x2a1b0, the run call at 0x1fd70, the
 * error path through 0x108e4, 0x10978, and 0x300be, and the epilogue at
 * 0x1be6c.
 *
 * The second argument selects the loading screen.  A value above 6 drops by 6
 * and sets the bit 0x08 in `DAT_0001d274`, and the function restores the saved
 * dword at the end.  The function registers the resource name `NET_SPACE`,
 * sets the screen index `DAT_000203bc`, copies the context word at `+8` to
 * `DAT_0001d2b8`, clears the 0x3e800-byte video buffer, fills the screen,
 * loads `load<arg2>H.pcx` from `gfx\graf.dat` into the pixel buffer at
 * `DAT_0001fad0`, and writes the palette at `DAT_0000cf5c`.
 *
 * A failed load runs the cleanup at 0x15150 and the function continues.  It
 * sets the palette and shows the message buffer, copies the pixels to
 * `DAT_00021528`, runs the two video swaps behind the flags 0x10 and 0x20,
 * draws the title pointer `DAT_0001fc3c` centered at y 0x50 with font 3 and
 * color 0, fills the screen again, and builds `map%d` at `DAT_0001d294`.
 *
 * The function publishes the two remaining arguments at `DAT_0001d2b0` and
 * `DAT_0001d2b4`, copies the context words at `+0x38` and `+0x48`, and reads
 * the 0x50-byte record selected by the screen index from `DAT_00020430` into
 * `DAT_0001d26c` when the low byte of `DAT_0001d260` holds none of the bits
 * 0x64, and stores 0 otherwise.  It then runs `race_cleanup`, runs the race,
 * and on a non-zero result reports the code through `bios_video(3)`,
 * `error_report`, and `exit`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern char *DAT_0001fc3c;
extern int DAT_0001d260;
extern unsigned int DAT_0001d274;
extern void *DAT_00002dec;
extern int DAT_000203bc;
extern int DAT_000203c4;
extern char DAT_0001d294[];
extern int DAT_0001d2a0;
extern int DAT_0001d2b0;
extern int DAT_0001d2b4;
extern int DAT_0001d2b8;
extern int DAT_0001d2c8;
extern int DAT_0001d2cc;
extern int DAT_0001d26c;
extern char DAT_00020430[];
extern void *DAT_00021528;
extern void *DAT_0001fad0;
extern char DAT_0000cf5c[];
extern void *DAT_000062ac;
extern int DAT_000021d8[];

extern void sub_2f326(void *record, char *name);
extern void sub_8bd54(int value);
extern void *memset(void *target, unsigned int count, int value);
extern void video_fill(void);
extern int pcx_load(char *file, char *member, void *pixels, char *palette);
extern void sub_15150(void);
extern void palette_set(char *palette);
extern void message_show(char *buffer);
extern void *memcpy(void *target, const void *source, unsigned int count);
extern void video_block_swap_a(void);
extern void video_block_swap_b(void);
extern void panel_draw_centered(char *text, int y, int font, int color);
extern int sprintf(char *buffer, const char *format, ...);
extern void race_cleanup(void);
extern int sub_1fd70(void);
extern void bios_video(int mode);
extern void error_report(int code);
extern void exit(int code);
extern void sub_1be6c(void);

void loading_screen(int arg1, int arg2, int arg3)
{
    char name[0x320];
    unsigned int saved;
    int adjust;
    int result;

    sub_2f326(DAT_000062ac, "NET_SPACE");
    if (arg2 > 6) {
        arg2 = arg2 - 6;
        saved = DAT_0001d274;
        DAT_0001d274 = DAT_0001d274 | 0x08;
        adjust = 1;
    } else {
        adjust = 0;
    }
    DAT_000203bc = arg2 - 1;
    sub_8bd54(DAT_000021d8[*(int *)((char *)DAT_00002dec + 8)]);
    DAT_0001d2b8 = *(int *)((char *)DAT_00002dec + 8);
    memset(DAT_00021528, 0x3e800, 0);
    video_fill();
    sprintf(name, "load%d%c.pcx", arg2, 0x48);
    result = pcx_load("gfx\\graf.dat", name, DAT_0001fad0, DAT_0000cf5c);
    if (result != 0) {
        sub_15150();
    }
    palette_set(DAT_0000cf5c);
    message_show(DAT_0000cf5c);
    memcpy(DAT_00021528, DAT_0001fad0, 0x3e800);
    if ((DAT_0001d274 & 0x10) != 0) {
        video_block_swap_a();
    }
    if ((DAT_0001d274 & 0x20) != 0) {
        video_block_swap_b();
    }
    panel_draw_centered(DAT_0001fc3c, 0x50, 3, 0);
    video_fill();
    sprintf(DAT_0001d294, "map%d", arg2);
    DAT_0001d2a0 = DAT_000203c4;
    DAT_0001d2b0 = arg1;
    DAT_0001d2b4 = arg3;
    DAT_0001d2c8 = *(int *)((char *)DAT_00002dec + 0x38);
    DAT_0001d2cc = *(int *)((char *)DAT_00002dec + 0x48);
    if ((DAT_0001d260 & 0x64) == 0) {
        DAT_0001d26c = *(int *)(DAT_00020430 + (arg2 - 1) * 0x50);
    } else {
        DAT_0001d26c = 0;
    }
    race_cleanup();
    result = sub_1fd70();
    if (result != 0) {
        bios_video(3);
        error_report(result);
        exit(result);
    }
    if (adjust != 0) {
        DAT_0001d274 = saved;
    }
    sub_1be6c();
}
