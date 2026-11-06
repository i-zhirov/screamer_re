/* The rebuilt `menu_frame` module.
 *
 * The original sits at linear 0x29f80 in STARTH.EXE and holds 151 instructions
 * and 560 bytes: the six register saves, the frame, the 0x30c-byte local area,
 * the `pcx_load` call at 0x1b5a4, the cleanup at 0x15150, the two
 * `video_present` calls at 0x27ec8, the `memcpy` call at 0x8c150, the two video
 * swaps at 0x1075f and 0x107a1, the `video_fill` call at 0x2aa98, the menu
 * constants, the two screen offsets, the three string lengths, the run init at
 * 0x2dff4, the `table_lookup` call at 0x1f718, and the epilogue.
 *
 * The function loads the member `menu1h.pcx` from `gfx\graf.dat` into the pixel
 * buffer `DAT_001fad0` with the palette at `DAT_0000cf5c`.  A failed load runs
 * the cleanup at 0x15150 and the function continues.
 *
 * The function maps the palette at scale 0, copies 0x3e800 bytes to the video
 * buffer `DAT_0021528`, runs the two video swaps behind the flags 0x10 and
 * 0x20 of `DAT_001d274`, and fills the screen.
 *
 * It then sets the menu constants: the frame flag `DAT_001d234`, the overlay
 * words `DAT_001fac0` and `DAT_001fac4`, the game state `DAT_00203b8`, the
 * frame step `DAT_0018e08` at 0.08, the counter `DAT_000007f9` at 8, the limit
 * `DAT_001fab8` at 0x35e, the flag `DAT_001d240` at 1, the factors
 * `DAT_0020378` and `DAT_0020380` at 0.6, the fade state `DAT_001fac8` at 1,
 * the menu value `DAT_001fb64` at -1, and the zero values at `DAT_001fb58`,
 * `DAT_001d224`, `DAT_001d248`, `DAT_00203a8`, `DAT_002039c`, `DAT_00203ac`,
 * `DAT_00203a0`, `DAT_00109a0`, `DAT_0020384`, `DAT_00109a4`, `DAT_0020388`,
 * `DAT_0020394`, `DAT_00109a8`, `DAT_002038c`, and `DAT_002037c`.
 *
 * The two screen offsets land at `DAT_0020824` and `DAT_0020828`.  Each one
 * centers half of the object field on the 0x140 screen width: the field at
 * `+0x134` for x minus 0x190, and the field at `+0x120` for y minus 0x320.
 *
 * The function keeps the longest of the three strings at `DAT_001fc1c`,
 * `DAT_001fc20`, and `DAT_001fc24` in `DAT_00203b4`, runs the init at 0x2dff4,
 * and calls `table_lookup` with 0.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern unsigned char *DAT_0015ccc;
extern float DAT_0018e08;
extern float DAT_001d224;
extern float DAT_0020824;
extern float DAT_0020828;
extern void *DAT_001fad0;
extern void *DAT_0021528;
extern unsigned int DAT_001d274;
extern int DAT_001d234;
extern int DAT_001fac0;
extern int DAT_001fac4;
extern int DAT_00203b8;
extern int DAT_000007f9;
extern int DAT_001fab8;
extern int DAT_001d240;
extern float DAT_0020378;
extern float DAT_0020380;
extern int DAT_001fac8;
extern char *DAT_001fc1c;
extern char *DAT_001fc20;
extern char *DAT_001fc24;
extern int DAT_001fb64;
extern int DAT_00203b0;
extern int DAT_00203a4;
extern int DAT_001fb58;
extern int DAT_001d248;
extern int DAT_00203a8;
extern int DAT_002039c;
extern int DAT_00203ac;
extern int DAT_00203a0;
extern int DAT_00109a0;
extern int DAT_0020384;
extern int DAT_00109a4;
extern int DAT_0020388;
extern int DAT_0020394;
extern int DAT_00109a8;
extern int DAT_002038c;
extern int DAT_002037c;
extern int DAT_00203b4;

extern int pcx_load(char *file, char *member, void *pixels, char *palette);
extern void sub_15150(void);
extern void video_present(float scale, int offset);
extern void *memcpy(void *target, const void *source, unsigned int count);
extern void video_block_swap_a(void);
extern void video_block_swap_b(void);
extern void video_fill(void);
extern void sub_2dff4(void);
extern int table_lookup(int index);

static int string_length(char *text)
{
    int count;

    count = 0;
    while (text[count] != 0) {
        count = count + 1;
    }
    return count;
}

void menu_frame(void)
{
    int count;
    int width;
    int height;

    if (pcx_load((char *)0xfe0, (char *)0xfd5, DAT_001fad0,
            (char *)0xcf5c) != 0) {
        sub_15150();
    }
    video_present(0.0f, 0);
    memcpy(DAT_0021528, DAT_001fad0, 0x3e800);
    if ((DAT_001d274 & 0x10) != 0) {
        video_block_swap_a();
    }
    if ((DAT_001d274 & 0x20) != 0) {
        video_block_swap_b();
    }
    video_fill();
    video_present(0.0f, 0);
    DAT_001d234 = 0;
    DAT_001fac0 = 0;
    DAT_001fac4 = 0;
    DAT_00203b8 = 0;
    DAT_0018e08 = 0.08f;
    DAT_000007f9 = 8;
    width = *(int *)(DAT_0015ccc + 0x134);
    DAT_0020824 = (float)(((0x140 - width / 2) / 2) - 0x190);
    height = *(int *)(DAT_0015ccc + 0x120);
    DAT_0020828 = (float)(((0x140 - height / 2) / 2) - 0x320);
    DAT_001fab8 = 0x35e;
    DAT_001d240 = 1;
    DAT_0020378 = 0.6f;
    DAT_0020380 = 0.6f;
    DAT_001fac8 = 1;
    DAT_001fb64 = -1;
    DAT_00203b0 = 0x1004;
    DAT_00203a4 = 0x1004;
    DAT_001fb58 = 0;
    DAT_001d224 = 0.0f;
    DAT_001d248 = 0;
    DAT_00203a8 = 0;
    DAT_002039c = 0;
    DAT_00203ac = 0;
    DAT_00203a0 = 0;
    DAT_00109a0 = 0;
    DAT_0020384 = 0;
    DAT_00109a4 = 0;
    DAT_0020388 = 0;
    DAT_0020394 = 0;
    DAT_00109a8 = 0;
    DAT_002038c = 0;
    DAT_002037c = 0;
    DAT_00203b4 = string_length(DAT_001fc1c);
    count = string_length(DAT_001fc20);
    if (count > DAT_00203b4) {
        DAT_00203b4 = count;
    }
    count = string_length(DAT_001fc24);
    if (count > DAT_00203b4) {
        DAT_00203b4 = count;
    }
    sub_2dff4();
    table_lookup(0);
}
