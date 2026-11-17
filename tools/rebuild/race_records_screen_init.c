/* The rebuilt `race_records_screen_init` module.
 *
 * The original sits at linear 0x2c954 in STARTH.EXE and holds 65 instructions
 * and 235 bytes up to the `ret` at 0x2ca3e.  The next function starts at
 * 0x2ca40, so the store record keeps the 236-byte extent and sets no separate
 * function size.  The identity check uses 65 instructions.
 *
 * The function is the state 6 init of the state init dispatcher at 0x2be94.
 * The matching state 6 update is `race_records_screen` at 0x2d4b8, and
 * `race_state_update` at 0x29564 calls the dispatcher, so the module takes the
 * section `race_frame callees`.
 *
 * The function reads the menu index `DAT_0011fb60`.  The entry table
 * `DAT_0011fb08` gives the row and the base table `DAT_00102de8` gives the
 * record base.  The dword at record + row * 16 + 0xc is the selected track and
 * goes to `DAT_0012098c`.
 *
 * The function sets `DAT_00120380` at 8.0, `DAT_0011fac8` at 1, and
 * `DAT_00120378` at 1.5, and runs the init at 0x2dff4.  It builds the member
 * `load<track + 1>H.pcx` and loads it from `gfx\graf.dat` into
 * `DAT_0011fad0` with the palette `DAT_0010cf5c` through `pcx_load` at
 * 0x1b5a4.  A non-zero result runs the cleanup at 0x15150.
 *
 * The function sets `DAT_00118e08` at 0.08 and `DAT_000007f9` at 8, builds
 * the palette shades through 0x258d0, presents the buffer with the scale 0 and
 * the offset 0 through 0x27ec8, copies 0x3e800 bytes to `DAT_00121528`
 * through `memcpy` at 0x8c150, fills the screen through 0x2aa98, presents
 * again, and clears the highlight row `DAT_00120990`.
 *
 * The state 6 update, the selected track, and the shared init tail with
 * `championship_screen_init` support the name `race_records_screen_init`.  The
 * original has no symbol, so the record keeps the name with the confidence
 * inferred.
 *
 * The function uses no floating-point instruction.  The original has the EBP
 * frame, so the module builds with `-of+`.
 */
extern int DAT_0011fb60;
extern int DAT_0011fb08[];
extern int DAT_00102de8[];
extern int DAT_0012098c;
extern float DAT_00120378;
extern float DAT_00120380;
extern int DAT_0011fac8;
extern void *DAT_0011fad0;
extern void *DAT_00121528;
extern unsigned char DAT_0010cf5c[];
extern float DAT_00118e08;
extern int DAT_000007f9;
extern int DAT_00120990;
extern char race_records_format[];
extern char race_records_archive[];

extern int sprintf(char *buffer, const char *format, ...);
extern int pcx_load(char *file, char *member, void *pixels, char *palette);
extern void sub_15150(void);
extern void palette_shade_build(unsigned char *palette);
extern void video_present(float scale, int offset);
extern void *memcpy(void *target, const void *source, unsigned int count);
extern void video_fill(void);
extern void sub_2dff4(void);

void race_records_screen_init(void)
{
    char member[0x80];
    int index;
    int row;
    int base;

    index = DAT_0011fb60;
    row = DAT_0011fb08[index];
    base = DAT_00102de8[index];
    DAT_0012098c = *(int *)(base + row * 16 + 0xc);
    DAT_00120380 = 8.0f;
    DAT_0011fac8 = 1;
    DAT_00120378 = 1.5f;
    sub_2dff4();
    sprintf(member, race_records_format, DAT_0012098c + 1, 'H');
    if (pcx_load(race_records_archive, member, DAT_0011fad0,
            (char *)DAT_0010cf5c) != 0) {
        sub_15150();
    }
    DAT_00118e08 = 0.08f;
    DAT_000007f9 = 8;
    palette_shade_build(DAT_0010cf5c);
    video_present(0.0f, 0);
    memcpy(DAT_00121528, DAT_0011fad0, 0x3e800);
    video_fill();
    video_present(0.0f, 0);
    DAT_00120990 = 0;
}
