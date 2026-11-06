/* The rebuilt `quit_confirm_screen` module.
 *
 * The original sits at linear 0x219ec in STARTH.EXE and holds 122 instructions
 * and 431 bytes up to the `ret` at 0x21b9a.  The next function starts later, so
 * the store record keeps the 432-byte extent and sets no function size.  The
 * identity check uses 122 instructions.
 *
 * The only direct caller at 0x1feb3 is the race frame loop 0x1fd70.  That loop
 * calls this function when the mode byte `DAT_001d260` bit 0x10 is clear and
 * the mode is not 2.  The loop is called by `loading_screen` at 0x2a514 and by
 * `race_results_screen` at 0x2a753.  The loading path treats a non-zero return
 * as an error and exits the game, so the return value is the quit answer.
 *
 * The function sets the screen mode 0x140 by 0x190 with the depth 0x104, starts
 * the sound service with the driver at object-3 offset 0x6350, runs the two
 * video swaps that `DAT_001d274` bits 0x10 and 0x20 gate, and clears the screen.
 * It fills the screen three times and draws the title line at `DAT_0018fe0` at
 * y 0x34 with the color 2.
 *
 * The loop polls the input when `DAT_001d2c0` and `DAT_001d2cc` are not zero,
 * copies the default data, and draws the two choice lines at `DAT_0018fe4` and
 * `DAT_0018fe8` at y 0x5c and 0x7a.  The selected line takes the color 2 and
 * the other line takes 0xe.  The highlight phase is bit 0x10 of
 * `DAT_000007f9`, and the exit flag selects the line.  The up keys
 * `DAT_0000004a` and `DAT_0000004d` set the exit flag to 0, and the down keys
 * `DAT_00000052` and `DAT_0000004f` set it to 1.  Each key group holds a latch,
 * so a held key runs its branch once, and the latch clears when the key group
 * releases.  A key change calls `table_lookup` with 6.
 *
 * The enter key `DAT_0000001e` returns the exit flag.  The escape key
 * `DAT_00000003` sets the exit flag to 1 and returns it.  The object-2 key
 * bytes arrive from the default data copy, so the harness holds them as
 * globals at the same offsets.
 *
 * The three text pointers at object-3 offsets 0x18fe0, 0x18fe4, and 0x18fe8
 * hold the title and the two choices.  The name `quit_confirm_screen` follows
 * from the two choices, the quit answer, and the exit in the loading path.  The
 * original holds no symbol for this function, so the record keeps the name with
 * the confidence inferred.
 *
 * The function uses no floating-point instruction.  The original has the EBP
 * frame, so the module builds with `-of+`.
 */
extern void screen_mode_set(unsigned int width, unsigned int height,
    unsigned int depth);
extern void sound_effects_update(int driver);
extern void video_block_swap_a(void);
extern void video_block_swap_b(void);
extern void rect_fill(int x, int y, int width, int height);
extern void video_fill(void);
extern void panel_draw_centered(char *text, int y, int font, int color);
extern int input_poll(void);
extern void default_data_copy(void);
extern int table_lookup(int index);

extern int DAT_001d274;
extern int DAT_001d2c0;
extern int DAT_001d2cc;
extern unsigned int DAT_000007f9;
extern char *DAT_0018fe0;
extern char *DAT_0018fe4;
extern char *DAT_0018fe8;
extern unsigned char DAT_0000001e;
extern unsigned char DAT_00000003;
extern unsigned char DAT_0000004a;
extern unsigned char DAT_0000004d;
extern unsigned char DAT_00000052;
extern unsigned char DAT_0000004f;

int quit_confirm_screen(void)
{
    int exit_flag;
    int up_latch;
    int down_latch;
    int input_bit;
    int color;
    int index;

    exit_flag = 0;
    down_latch = 0;
    up_latch = down_latch;

    screen_mode_set(0x140, 0x190, 0x104);
    sound_effects_update(0x6350);
    if ((DAT_001d274 & 0x10) != 0) {
        video_block_swap_a();
    }
    if ((DAT_001d274 & 0x20) != 0) {
        video_block_swap_b();
    }
    video_fill();

    for (index = 0; index < 3; index++) {
        rect_fill(0, 0, 0x280, 0x190);
    }
    panel_draw_centered(DAT_0018fe0, 0x34, 0, 2);

    for (;;) {
        if (DAT_001d2c0 != 0 && DAT_001d2cc != 0) {
            input_poll();
        }
        input_bit = DAT_000007f9 & 0x10;
        default_data_copy();

        if (exit_flag == 0 && input_bit != 0) {
            color = 2;
        } else {
            color = 0xe;
        }
        panel_draw_centered(DAT_0018fe4, 0x5c, 0, color);

        if (exit_flag != 0 && input_bit != 0) {
            color = 2;
        } else {
            color = 0xe;
        }
        panel_draw_centered(DAT_0018fe8, 0x7a, 0, color);
        video_fill();

        if (DAT_0000004a != 0 || DAT_0000004d != 0) {
            if (up_latch == 0) {
                exit_flag = 0;
                table_lookup(6);
                up_latch = 1;
            }
        } else {
            up_latch = 0;
        }

        if (DAT_00000052 != 0 || DAT_0000004f != 0) {
            if (down_latch == 0) {
                exit_flag = 1;
                table_lookup(6);
                down_latch = 1;
            }
        } else {
            down_latch = 0;
        }

        if (DAT_0000001e != 0) {
            return exit_flag;
        }
        if (DAT_00000003 == 0) {
            continue;
        }
        exit_flag = 1;
        return exit_flag;
    }
}
