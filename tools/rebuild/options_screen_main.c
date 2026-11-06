/* The test harness for the rebuilt `options_screen` module.
 *
 * The harness holds the menu state, the three volume slots, the seven state
 * positions, and the callee stubs.  The `sound_slots_update` stub sets the
 * input flags of the next frame from a script and sets the escape key
 * `DAT_00000003` after a configured number of frames, so every case leaves the
 * loop.
 *
 * The harness runs eleven paths:
 *
 *     0  the first frame with zero volumes: the two clear calls, the sound
 *        marker, the music start, the escape key, and the two view moves
 *     1  two frames with the volumes 1000: the fades to 836 and 672, the exit
 *        apply, and the final view move from the globals
 *     2  state 4: the CD volume falls from 1000 to 520
 *     3  state 5: the CD volume rises from 32700 and clips at 32767
 *     4  state 6: the volume index cycles through CD, CD, FX, MU, and CD
 *     5  state 3: the selection rises from 3 to 4
 *     6  state 2: the selection falls from 3 to 2
 *     7  the left key: the state falls and the key 0x4b clears
 *     8  the right key: the state rises and the key 0x4d clears
 *     9  the list path: four formatted rows at the step 0x1a
 *     a  the empty list path: the sound driver and the title draw
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

void options_screen(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static int stop_after;
static int slot_calls;
static unsigned char script_4f[16];
static unsigned char script_1e[16];
static unsigned char script_4d[16];
static int clear_calls;
static int clear_keys[8];
static int view_calls;
static int view_a[8];
static int view_b[8];
static int view_c[8];
static int rect_calls;
static int sound_calls;
static int music_calls;
static int music_song;
static int reset_calls;
static int copy_calls;
static int blit_calls;
static int list_begin_calls;
static int format_calls;
static int format_values[16];
static int draw_calls;
static int draw_x[16];
static int draw_y[16];
static char *draw_text[16];
static int draw_font[16];
static int draw_color[16];
static int driver_calls;
static int driver_value;
static int poll_calls;
static int bar_calls;
static int bar_value[8];
static char bar_label0[8];
static char bar_label1[8];
static int fill_calls;
static int apply_calls;
static int apply_value;
static int apply_flag;
static int release_calls;

unsigned char DAT_00000003;
unsigned char DAT_0000001e;
unsigned char DAT_0000004d;
unsigned char DAT_0000004f;
unsigned char DAT_000007fd;
unsigned int DAT_000007f9;
int DAT_0016684;
int DAT_0016688;
int DAT_001668e;
int DAT_0016692;
int DAT_0016696;
int DAT_001669a;
void *DAT_0018f14;
void *DAT_0018fd4;
void *DAT_0018fd8;
void *DAT_0018fdc;
int DAT_001cb18;
int DAT_001cb1c;
int DAT_001cb20;
int DAT_001d260;
int DAT_001d2bc;
int DAT_001d2c0;
int DAT_001d2c4;
int DAT_001d2cc;
void *DAT_0021528;
char options_label_cd[] = "CD";
char options_label_fx[] = "FX";
char options_label_mu[] = "MU";
float options_fade_rate;
float options_rise_rate;
float options_min_delta;
double options_time_scale;

static char title0[] = "title0";
static char title1[] = "title1";
static char title2[] = "title2";

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static char *put_decimal(char *out, int value)
{
    char digits[12];
    int count;
    unsigned int number;

    count = 0;
    if (value < 0) {
        *out++ = '-';
        number = (unsigned int)(-value);
    } else {
        number = (unsigned int)value;
    }
    do {
        digits[count] = (char)('0' + (number % 10));
        number = number / 10;
        count = count + 1;
    } while (number != 0);
    while (count > 0) {
        count = count - 1;
        *out++ = digits[count];
    }
    return out;
}

void rect_fill(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    rect_calls = rect_calls + 1;
}

void menu_state_reset(int state, int value)
{
    (void)state;
    (void)value;
    reset_calls = reset_calls + 1;
}

void sound_marker(void)
{
    sound_calls = sound_calls + 1;
}

void music_play(void *song)
{
    music_calls = music_calls + 1;
    music_song = (int)song;
}

void view_move(int a, int b, int c)
{
    if (view_calls < 8) {
        view_a[view_calls] = a;
        view_b[view_calls] = b;
        view_c[view_calls] = c;
    }
    view_calls = view_calls + 1;
}

void default_data_copy(void)
{
    copy_calls = copy_calls + 1;
}

void sprite_blit(void *source, void *dest)
{
    (void)source;
    (void)dest;
    blit_calls = blit_calls + 1;
}

void input_clear(int key)
{
    if (clear_calls < 8) {
        clear_keys[clear_calls] = key;
    }
    clear_calls = clear_calls + 1;
}

void menu_list_begin(void)
{
    list_begin_calls = list_begin_calls + 1;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    int value;

    (void)format;
    va_start(arguments, format);
    value = va_arg(arguments, int);
    va_end(arguments);
    if (format_calls < 16) {
        format_values[format_calls] = value;
    }
    format_calls = format_calls + 1;
    put_decimal(buffer, value);
    return 0;
}

void text_draw(int x, int y, char *value, int font, int color)
{
    if (draw_calls < 16) {
        draw_x[draw_calls] = x;
        draw_y[draw_calls] = y;
        draw_text[draw_calls] = value;
        draw_font[draw_calls] = font;
        draw_color[draw_calls] = color;
    }
    draw_calls = draw_calls + 1;
}

void sound_driver_set(int value)
{
    driver_calls = driver_calls + 1;
    driver_value = value;
}

void menu_poll(void)
{
    poll_calls = poll_calls + 1;
}

void sound_slots_update(void)
{
    slot_calls = slot_calls + 1;
    if (slot_calls < 16) {
        DAT_0000004f = script_4f[slot_calls];
        DAT_0000001e = script_1e[slot_calls];
        DAT_0000004d = script_4d[slot_calls];
    }
    if (slot_calls == stop_after) {
        DAT_00000003 = 1;
    }
}

void volume_bar_draw(int value, char *label)
{
    if (bar_calls < 8) {
        bar_value[bar_calls] = value;
        if (label == options_label_cd) {
            bar_label0[bar_calls] = 'C';
            bar_label1[bar_calls] = 'D';
        } else if (label == options_label_fx) {
            bar_label0[bar_calls] = 'F';
            bar_label1[bar_calls] = 'X';
        } else if (label == options_label_mu) {
            bar_label0[bar_calls] = 'M';
            bar_label1[bar_calls] = 'U';
        }
    }
    bar_calls = bar_calls + 1;
}

void video_fill(void)
{
    fill_calls = fill_calls + 1;
}

void input_apply(int value, int flag)
{
    apply_calls = apply_calls + 1;
    apply_value = value;
    apply_flag = flag;
}

void input_apply_release(void)
{
    release_calls = release_calls + 1;
}

static void set_bits_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *p = value.f;
}

static void set_bits_double(double *p, unsigned int low, unsigned int high)
{
    union {
        double d;
        unsigned int u[2];
    } value;

    value.u[0] = low;
    value.u[1] = high;
    *p = value.d;
}

static void reset_case(void)
{
    int index;

    set_bits_float(&options_fade_rate, 0x46800000);
    set_bits_float(&options_rise_rate, 0x473b8000);
    set_bits_float(&options_min_delta, 0x3c23d70a);
    set_bits_double(&options_time_scale, 0x7b14ae47, 0x3f847ae1);

    DAT_00000003 = 0;
    DAT_0000001e = 0;
    DAT_0000004d = 0;
    DAT_0000004f = 0;
    DAT_000007fd = 0;
    DAT_000007f9 = 0;
    DAT_0016684 = 0;
    DAT_0016688 = 0;
    DAT_001668e = 0;
    DAT_0016692 = 0;
    DAT_0016696 = 0;
    DAT_001669a = 0;
    DAT_0018f14 = 0;
    DAT_0018fd4 = title0;
    DAT_0018fd8 = title1;
    DAT_0018fdc = title2;
    DAT_001cb18 = 0;
    DAT_001cb1c = 0;
    DAT_001cb20 = 0;
    DAT_001d260 = 0;
    DAT_001d2bc = 0;
    DAT_001d2c0 = 0;
    DAT_001d2c4 = 0;
    DAT_001d2cc = 0;
    DAT_0021528 = 0;

    stop_after = 1000;
    slot_calls = 0;
    clear_calls = 0;
    view_calls = 0;
    rect_calls = 0;
    sound_calls = 0;
    music_calls = 0;
    music_song = 0;
    reset_calls = 0;
    copy_calls = 0;
    blit_calls = 0;
    list_begin_calls = 0;
    format_calls = 0;
    draw_calls = 0;
    driver_calls = 0;
    driver_value = 0;
    poll_calls = 0;
    bar_calls = 0;
    fill_calls = 0;
    apply_calls = 0;
    apply_value = 0;
    apply_flag = 0;
    release_calls = 0;
    for (index = 0; index < 8; index++) {
        clear_keys[index] = 0;
        view_a[index] = 0;
        view_b[index] = 0;
        view_c[index] = 0;
        bar_value[index] = 0;
        bar_label0[index] = 0;
        bar_label1[index] = 0;
    }
    for (index = 0; index < 16; index++) {
        script_4f[index] = 0;
        script_1e[index] = 0;
        script_4d[index] = 0;
        format_values[index] = 0;
        draw_x[index] = 0;
        draw_y[index] = 0;
        draw_text[index] = 0;
        draw_font[index] = 0;
        draw_color[index] = 0;
    }
}

static int view_is(int index, int a, int b, int c)
{
    return view_calls > index && view_a[index] == a && view_b[index] == b
        && view_c[index] == c;
}

static int draw_is(int index, int x, int y, int font, int color)
{
    return draw_calls > index && draw_x[index] == x && draw_y[index] == y
        && draw_font[index] == font && draw_color[index] == color;
}

int main(void)
{
    reset_case();
    DAT_00000003 = 1;
    options_screen();
    put(rect_calls == 2 && sound_calls == 1 && music_calls == 1
        && music_song == 0x62b0 && clear_calls == 1 && clear_keys[0] == 1
        && blit_calls == 1 && apply_calls == 0 && view_calls == 2
        && view_is(0, 0, 0, 0) && view_is(1, 0, 0, 0) ? '0' : 'x');

    reset_case();
    DAT_001d2bc = 1000;
    DAT_001d2c0 = 1000;
    DAT_001d2c4 = 1000;
    DAT_001d2cc = 1;
    stop_after = 2;
    options_screen();
    put(sound_calls == 0 && music_calls == 0 && clear_calls == 1
        && clear_keys[0] == 1 && apply_calls == 1 && apply_value == 0
        && apply_flag == 1 && view_calls == 4
        && view_is(0, 1000, 1000, 1000)
        && view_is(1, 836, 836, 836)
        && view_is(2, 672, 672, 672)
        && view_is(3, 1000, 1000, 1000) ? '1' : 'x');

    reset_case();
    DAT_001d2bc = 1000;
    DAT_001d2c0 = 1000;
    DAT_001d2c4 = 1000;
    DAT_001668e = 4;
    DAT_0016692 = 6;
    DAT_0016696 = 4;
    DAT_0000004f = 1;
    script_4f[1] = 1;
    script_4f[2] = 1;
    script_4f[3] = 1;
    script_1e[3] = 1;
    stop_after = 4;
    options_screen();
    put(DAT_001d2c0 == 520 && bar_calls == 1 && bar_value[0] == 520
        && bar_label0[0] == 'C' && bar_label1[0] == 'D'
        && clear_calls == 6 && clear_keys[4] == 0x1c
        && clear_keys[5] == 1 ? '2' : 'x');

    reset_case();
    DAT_001d2bc = 1000;
    DAT_001d2c0 = 32700;
    DAT_001d2c4 = 1000;
    DAT_001668e = 4;
    DAT_0016692 = 6;
    DAT_0016696 = 5;
    DAT_0000004f = 1;
    script_4f[1] = 1;
    script_4f[2] = 1;
    script_4f[3] = 1;
    script_4f[4] = 1;
    script_1e[4] = 1;
    stop_after = 5;
    options_screen();
    put(DAT_001d2c0 == 32767 && bar_calls == 2 && bar_value[0] == 32700
        && bar_value[1] == 32767 ? '3' : 'x');

    reset_case();
    DAT_001d2bc = 1000;
    DAT_001d2c0 = 1000;
    DAT_001d2c4 = 1000;
    DAT_001668e = 4;
    DAT_0016692 = 6;
    DAT_0016696 = 6;
    DAT_0000004f = 1;
    script_4f[1] = 1;
    script_4f[2] = 1;
    script_4f[3] = 1;
    script_4f[4] = 1;
    script_4f[5] = 1;
    script_1e[5] = 1;
    script_1e[6] = 1;
    script_1e[7] = 1;
    stop_after = 8;
    options_screen();
    put(bar_calls == 5 && bar_label0[0] == 'C' && bar_label0[1] == 'C'
        && bar_label0[2] == 'F' && bar_label0[3] == 'M'
        && bar_label0[4] == 'C' && bar_value[0] == 1000
        && bar_value[1] == 1000 && bar_value[2] == 1000
        && bar_value[3] == 1000 && bar_value[4] == 1000 ? '4' : 'x');

    reset_case();
    DAT_0016696 = 3;
    DAT_001668e = 2;
    DAT_0016692 = 5;
    DAT_0016684 = 1;
    DAT_001d2cc = 1;
    DAT_0000004f = 1;
    script_4f[1] = 1;
    script_4f[2] = 1;
    script_1e[2] = 1;
    stop_after = 3;
    options_screen();
    put(apply_calls == 1 && apply_value == 4 && apply_flag == 1
        && clear_calls == 5 && clear_keys[3] == 0x1c
        && clear_keys[4] == 1 ? '5' : 'x');

    reset_case();
    DAT_0016696 = 3;
    DAT_001668e = 2;
    DAT_0016692 = 5;
    DAT_0016684 = 1;
    DAT_001d2cc = 1;
    DAT_0000004f = 1;
    script_4f[1] = 1;
    script_1e[1] = 1;
    stop_after = 2;
    options_screen();
    put(apply_calls == 1 && apply_value == 2 && apply_flag == 1
        && clear_calls == 4 && clear_keys[2] == 0x1c
        && clear_keys[3] == 1 ? '6' : 'x');

    reset_case();
    DAT_0000004f = 1;
    script_4d[1] = 1;
    stop_after = 1;
    options_screen();
    put(clear_calls == 3 && clear_keys[0] == 0x4d
        && clear_keys[1] == 0x4b && clear_keys[2] == 1 ? '7' : 'x');

    reset_case();
    DAT_0000004f = 1;
    DAT_00000003 = 1;
    options_screen();
    put(clear_calls == 2 && clear_keys[0] == 0x4d && clear_keys[1] == 1
        ? '8' : 'x');

    reset_case();
    DAT_0016684 = 1;
    DAT_001668e = 0;
    DAT_0016692 = 3;
    DAT_0016696 = 1;
    stop_after = 1;
    options_screen();
    put(list_begin_calls == 1 && format_calls == 4 && format_values[0] == 0
        && format_values[1] == 1 && format_values[2] == 2
        && format_values[3] == 3 && draw_calls == 4
        && draw_is(0, 0x80, 0x9c, 5, 9)
        && draw_is(1, 0x9a, 0x9c, 5, 9)
        && draw_is(2, 0xb4, 0x9c, 5, 9)
        && draw_is(3, 0xce, 0x9c, 5, 9) ? '9' : 'x');

    reset_case();
    DAT_0016684 = 1;
    DAT_001669a = 1;
    stop_after = 1;
    options_screen();
    put(driver_calls == 1 && driver_value == 0 && poll_calls == 1
        && draw_calls == 1 && draw_text[0] == DAT_0018fd4
        && draw_is(0, 0x80, 0x8c, 5, 3) ? 'a' : 'x');

    put('D');
    quit();
    return 0;
}
