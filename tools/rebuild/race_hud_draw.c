/* The rebuilt `race_hud_draw` module.
 *
 * The original sits at linear 0x23420 in STARTH.EXE and holds 173 instructions
 * and 633 bytes up to the `ret` at 0x23698.  The next function starts at
 * 0x2369c, so the store record keeps the 636-byte extent and sets no function
 * size.  The identity check uses 173 instructions.
 *
 * The only caller at 0x2001a is the frame update at 0x1ff70, which calls the
 * function after the five view groups.  No named function sits between the
 * frame update and this function, so the record takes the section Program
 * functions.
 *
 * The function formats the race clock `0x1100a8` into the global text at
 * `0x101f98`.  A set replay flag `0x118e6c` stops the rest of the draw; the
 * frame counter `0x1007fd` bit 0x80 then adds the replay label `R`.  Otherwise
 * the function draws the gauge panel, the lap value `0x1100ac` centered at the
 * top, the lap number from `0x10ffa0` against `0x11d2b0`, the counter
 * `0x11d2b0` plus one, and the offset `0x10ffa4`.  It then formats the lap
 * clock `0x1100b0`, draws the two clocks and the static clock label at
 * `0x101f8c`, and draws the race radar at `0x23a94`.
 *
 * The original draws through `text_draw` at 0x25020, `panel_draw_centered` at
 * 0x2e7cc, `text_measure` at 0x254a8, `sprintf` at 0x4d32c, the gauge panel at
 * 0x2369c, and the radar at 0x23a94.  The strings and the format strings sit at
 * object-3 offsets 0xdd0 through 0xdef and 0x1f8c through 0x1f98.
 *
 * The original truncates each float through the helper at 0x4d9f4.  The shared
 * `fpu_stub.asm` leaves round-to-nearest, so this harness uses the local
 * truncating `__CHP` stub.  The 100.0 scale arrives through a global, because
 * the minimal image does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern float race_clock;
extern float race_lap_number;
extern float race_lap_clock;
extern int race_lap_display;
extern int race_lap_offset;
extern int race_position;
extern int race_replay;
extern unsigned char race_frame_counter;

extern char race_time_format[];
extern char race_count_format[];
extern char race_value_format[];
extern char race_lap_format[];
extern char race_offset_format[];
extern char race_best_format[];
extern char race_replay_label[];
extern char race_clock_text[];
extern char race_clock_label[];

extern float race_centi_scale;

extern void race_gauges(void);
extern void race_radar(void);
extern void panel_draw_centered(char *text, int y, int font, int color);
extern void text_draw(int x, int y, char *text, int font, int color);
extern void text_measure(int *width, int *height, char *text, int font);
extern int sprintf(char *buffer, const char *format, ...);

static void format_time(char *buffer, float value, char *format)
{
    int whole;
    int minutes;
    int seconds;
    int centis;

    whole = (int)value;
    minutes = whole / 60;
    seconds = whole % 60;
    centis = (int)((value - (float)whole) * race_centi_scale);
    sprintf(buffer, format, minutes, seconds, centis);
}

void race_hud_draw(void)
{
    char buffer[0x100];
    int width;
    int height;
    int value;

    format_time(race_clock_text, race_clock, race_time_format);

    if (race_replay != 0) {
        if ((race_frame_counter & 0x80) != 0) {
            text_draw(0x10, 0x154, race_replay_label, 0, 6);
        }
        return;
    }

    race_gauges();

    sprintf(buffer, race_count_format, (int)race_lap_number);
    panel_draw_centered(buffer, 0x10, 6, 2);

    value = race_lap_display;
    if (value <= race_position) {
        value = value + 1;
    }
    sprintf(buffer, race_value_format, value, 0x3a);
    text_draw(0x10, 0x16, buffer, 1, 5);
    text_measure(&width, &height, buffer, 1);

    sprintf(buffer, race_lap_format, race_position + 1);
    text_draw(width + 0xa, 0x28, buffer, 4, 5);

    sprintf(buffer, race_offset_format, race_lap_offset);
    text_draw(0x228, 0x16, buffer, 1, 5);

    format_time(buffer, race_lap_clock, race_best_format);
    text_draw(0x10, 0x70, race_clock_text, 3, 2);
    text_draw(0x10, 0x90, buffer, 3, 2);
    text_draw(0x10, 0xb0, race_clock_label, 3, 2);

    race_radar();
}
