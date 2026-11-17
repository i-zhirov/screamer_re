/* The test harness for the rebuilt `view_fade_out` module.
 *
 * The harness holds the phase counter `DAT_000007f9`, the three view offsets
 * `DAT_001d2c0`, `DAT_001d2bc`, and `DAT_001d2c4`, and the three callee stubs.
 * The `phase_delta` stub returns a scripted delta and advances the phase counter
 * by one, so the module must pass the saved start value and the current value.
 * The `view_move` stub records the three arguments.  The `video_present` stub
 * records the scale and the offset.  The harness runs six paths:
 *
 *     0  a non-zero argument: the view moves and the screen fades over three
 *        steps with the fractions 0, 0.5, and 1.02
 *     1  a zero argument: the view stays still and the screen still fades
 *     2  negative view offsets: the scaled values truncate toward zero
 *     3  the exact fraction 1.0: the loop runs one more step with the scale 0
 *     4  a delta above 0xffff: the low 16 bits give the fraction 1.0
 *     5  one step above 1.0: the loop ends at once after the first present
 *     D  the exit marker
 *
 * Every path also checks the saved start value, the current value, the present
 * offset 0, and the final present at the scale 0.
 *
 * The two doubles 0.01 and 2.0 arrive through the globals `view_fade_out_rate`
 * and `view_fade_out_step`, because the minimal image does not load the
 * constant segment.  The harness sets them from their bit patterns.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void view_fade_out(int move_view);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_000007f9;
int DAT_001d2bc;
int DAT_001d2c0;
int DAT_001d2c4;
double view_fade_out_rate;
double view_fade_out_step;

static char text[8];

static int phase_base;
static int phase_step;
static int phase_bad;
static int script[16];
static int script_length;
static int script_index;

static int view_calls;
static int view_x[16];
static int view_y[16];
static int view_z[16];

static int present_calls;
static float present_scale[16];
static int present_offset[16];

int phase_delta(int start, int now)
{
    int value;
    int index;

    if (start != phase_base) {
        phase_bad = 1;
    }
    if (now != DAT_000007f9) {
        phase_bad = 1;
    }
    index = script_index;
    value = index < script_length ? script[index] : script[script_length - 1];
    script_index = index + 1;
    DAT_000007f9 = DAT_000007f9 + phase_step;
    return value;
}

void view_move(int x, int y, int z)
{
    int index;

    index = view_calls;
    if (index < 16) {
        view_x[index] = x;
        view_y[index] = y;
        view_z[index] = z;
    }
    view_calls = index + 1;
}

void video_present(float scale, int offset)
{
    int index;

    index = present_calls;
    if (index < 16) {
        present_scale[index] = scale;
        present_offset[index] = offset;
    }
    present_calls = index + 1;
}

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_bits_double(double *p, unsigned int lo, unsigned int hi)
{
    union {
        double d;
        unsigned int u[2];
    } value;

    value.u[0] = lo;
    value.u[1] = hi;
    *p = value.d;
}

static void reset(int phase, int step, int x, int y, int z)
{
    int index;

    phase_base = phase;
    phase_step = step;
    phase_bad = 0;
    script_length = 0;
    script_index = 0;
    for (index = 0; index < 16; index++) {
        script[index] = 0;
        view_x[index] = 0;
        view_y[index] = 0;
        view_z[index] = 0;
        present_scale[index] = -1.0f;
        present_offset[index] = -1;
    }
    view_calls = 0;
    present_calls = 0;
    DAT_000007f9 = phase;
    DAT_001d2c0 = x;
    DAT_001d2bc = y;
    DAT_001d2c4 = z;
}

static void add_delta(int value)
{
    script[script_length] = value;
    script_length = script_length + 1;
}

static int view_is(int index, int x, int y, int z)
{
    return index < view_calls
        && view_x[index] == x
        && view_y[index] == y
        && view_z[index] == z;
}

static int present_is(int index, float scale, int offset)
{
    return index < present_calls
        && present_scale[index] == scale
        && present_offset[index] == offset;
}

static int final_present_zero(void)
{
    return present_is(present_calls - 1, 0.0f, 0);
}

int main(void)
{
    set_bits_double(&view_fade_out_rate, 0x47ae147b, 0x3f847ae1);
    set_bits_double(&view_fade_out_step, 0x00000000, 0x40000000);
    reset(1000, 1, 1000, 2000, 500);
    add_delta(0);
    add_delta(25);
    add_delta(51);
    view_fade_out(1);
    put(view_calls == 3 && present_calls == 4 && phase_bad == 0
        && view_is(0, 1000, 2000, 500)
        && view_is(1, 1000, 2000, 500)
        && view_is(2, 500, 1000, 250)
        && present_is(0, 1.0f, 0)
        && present_is(1, 1.0f, 0)
        && present_is(2, 0.5f, 0)
        && final_present_zero() ? '0' : 'x');

    reset(20, 1, 1000, 2000, 500);
    add_delta(51);
    view_fade_out(0);
    put(view_calls == 0 && present_calls == 2 && phase_bad == 0
        && present_is(0, 1.0f, 0)
        && final_present_zero() ? '1' : 'x');

    reset(30, 1, 101, -101, 7);
    add_delta(25);
    add_delta(51);
    view_fade_out(1);
    put(view_calls == 2 && present_calls == 3 && phase_bad == 0
        && view_is(0, 101, -101, 7)
        && view_is(1, 50, -50, 3)
        && final_present_zero() ? '2' : 'x');

    reset(40, 1, 1000, 2000, 500);
    add_delta(50);
    add_delta(51);
    view_fade_out(1);
    put(view_calls == 2 && present_calls == 3 && phase_bad == 0
        && view_is(0, 1000, 2000, 500)
        && view_is(1, 0, 0, 0)
        && present_is(1, 0.0f, 0)
        && final_present_zero() ? '3' : 'x');

    reset(50, 1, 100, 200, 300);
    add_delta(0x10032);
    add_delta(51);
    view_fade_out(1);
    put(view_calls == 2 && present_calls == 3 && phase_bad == 0
        && view_is(0, 100, 200, 300)
        && view_is(1, 0, 0, 0)
        && present_is(1, 0.0f, 0)
        && final_present_zero() ? '4' : 'x');

    reset(60, 1, -300, -200, -100);
    add_delta(51);
    view_fade_out(1);
    put(view_calls == 1 && present_calls == 2 && phase_bad == 0
        && view_is(0, -300, -200, -100)
        && present_is(0, 1.0f, 0)
        && final_present_zero() ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
