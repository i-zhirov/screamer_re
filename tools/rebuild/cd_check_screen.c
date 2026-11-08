/* The rebuilt `cd_check_screen` module.
 *
 * The original sits at linear 0x15874 in STARTH.EXE and holds 69 instructions
 * and 269 bytes up to the `ret` at 0x15980.  The next function starts at
 * 0x15984, so the store record keeps the 272-byte extent and sets no separate
 * function size.  The identity check uses 69 instructions.
 *
 * The only direct caller is `race_end` at 0x279a4.  It calls this function at
 * 0x279f4 when the flag at object-3 offset 0x18e3c is zero, and then sets the
 * flag to 1.  `race_loop` calls `race_end` after the loop exit test, so the
 * module takes the section `race_loop callees`.
 *
 * The helper at 0x157b8 reads the real-mode CD driver through 0x26014 and
 * compares four fields of the response with the texts `PC`, `SCREAMER`, `VIE`,
 * and `SCREAMER`.  It returns 0 when the correct Screamer CD is present and 1
 * otherwise.  This function returns at once when the check succeeds, so the
 * screen runs only when the CD is missing or wrong.
 *
 * The screen path first stops the CD audio through 0x264ec, clears the dword
 * at object-3 offset 0x166ac, and gets the `OPT_PCX` resource of 0x3e800
 * bytes through 0x2eed0 into the pixel buffer at 0x1fad0.  A zero result runs
 * the memory error report at 0x2edad and the path continues.
 *
 * The function clears the 0x3e800-byte video buffer at 0x21528 through 0x8c160
 * and fills the screen through 0x2aa98.  It loads `menu1h.pcx` from
 * `gfx\graf.dat` into the pixel buffer with the palette at 0xcf5c through
 * 0x1b5a4.  A non-zero result runs the cleanup at 0x15150.
 *
 * The function copies the pixels to the video buffer through 0x8c150, draws
 * the text pointer at object-3 offset 0x18ff4 centered at y 0x3c with font 0
 * and color 2 through 0x2e7cc, fills the screen again, sets the palette
 * through 0x81a05, and stops the CD audio through 0x262a8 with 0.
 *
 * The function waits for the audio flag at 0x16688 to go from 0 to 1 and back
 * to 0, polling 0x25f90.  It then waits while the CD check at 0x157b8 still
 * fails, so the user can insert the correct disc.  After that it sets the word
 * at 0x1669e to 1, waits while the audio flag is set, and resets the CD audio
 * through 0x262fc.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  It uses no
 * floating-point instruction, so it links no FPU stub.
 */
extern void *DAT_000062ac;
extern int DAT_000166ac;
extern void *DAT_0001fad0;
extern void *DAT_00021528;
extern char *DAT_00018ff4;
extern int DAT_00016688;
extern short DAT_0001669e;
extern char DAT_0000cf5c[];

extern int cd_check(void);
extern void cd_audio_stop(void);
extern void *resource_get(void *archive, const char *name, int size);
extern void memory_error_report(void *archive);
extern void *memset(void *target, unsigned int count, int value);
extern void video_fill(void);
extern int pcx_load(
    const char *file, const char *member, void *pixels, char *palette);
extern void sub_15150(void);
extern void *memcpy(void *target, const void *source, unsigned int count);
extern void panel_draw_centered(const char *text, int y, int font, int color);
extern void palette_set(char *palette);
extern void sound_driver_set(int value);
extern void cd_audio_poll(void);
extern void cd_audio_reset(void);

void cd_check_screen(void)
{
    if (cd_check() == 0) {
        return;
    }
    cd_audio_stop();
    DAT_000166ac = 0;
    DAT_0001fad0 = resource_get(DAT_000062ac, "OPT_PCX", 0x3e800);
    if (DAT_0001fad0 == 0) {
        memory_error_report(DAT_000062ac);
    }
    memset(DAT_00021528, 0x3e800, 0);
    video_fill();
    if (pcx_load("gfx\\graf.dat", "menu1h.pcx", DAT_0001fad0,
            DAT_0000cf5c) != 0) {
        sub_15150();
    }
    memcpy(DAT_00021528, DAT_0001fad0, 0x3e800);
    panel_draw_centered(DAT_00018ff4, 0x3c, 0, 2);
    video_fill();
    palette_set(DAT_0000cf5c);
    sound_driver_set(0);
    while (DAT_00016688 == 0) {
        cd_audio_poll();
    }
    while (DAT_00016688 != 0) {
        cd_audio_poll();
    }
    while (cd_check() != 0) {
        cd_audio_poll();
    }
    DAT_0001669e = 1;
    while (DAT_00016688 != 0) {
        cd_audio_poll();
    }
    cd_audio_reset();
}
