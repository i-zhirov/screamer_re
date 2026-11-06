/* The test harness for the rebuilt `frame_timer` module.
 *
 * The harness holds the frame counter, the five timing floats, the filter table
 * pointer, the phase word, and the eight constant globals.  It sets the exact
 * constant bit patterns before the first call.  Each path sets the counter to a
 * value of 2 or more, because the module waits for the counter.  The harness
 * runs six paths:
 *
 *     0  a plain delta: the counter clears, the raw delta is 1.0, the smooth
 *        delta is 0.145, the frame rate is 6, and the phase is 0.0435
 *     1  the upper clamp: the counter 1000 gives the raw delta 10.0, the smooth
 *        delta clamps at 0.2, the truncating frame rate is 4, and the phase
 *        wraps from 0.95 to 0.01
 *     2  the lower clamp: the counter 2 gives the raw delta 0.02, the smooth
 *        delta clamps at 0.02, and the frame rate is 50
 *     3  a wrap: the smooth delta is 0.093, the phase 1.0 wraps to 0.0279, and
 *        the raw delta adds to the accumulated timing
 *     4  a phase below 1.0: the phase 0.9 advances to 0.9555 and does not wrap
 *     5  a phase just below the wrap: the phase 0.95 advances past 1.0 and
 *        wraps to 0.0055
 *     D  the exit marker
 *
 * The harness compares the exact result bit patterns, because the compiler
 * places float literals in the data segment.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void frame_timer(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_000007f9;
float DAT_00118e04;
float DAT_00118e08;
float DAT_00118e0c;
float DAT_00118e10;
float DAT_00118e14;
unsigned int DAT_0010cf08;
unsigned int DAT_0002153c;
double frame_delta_scale;
float frame_delta_gain;
float frame_delta_norm;
double frame_delta_high;
double frame_delta_low;
double frame_phase_step;
float frame_phase_wrap;
float frame_phase_scale;

static char text[8];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_bits_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *p = v.f;
}

static void set_bits_double(double *p, unsigned int lo, unsigned int hi)
{
    union {
        double d;
        unsigned int u[2];
    } v;

    v.u[0] = lo;
    v.u[1] = hi;
    *p = v.d;
}

static void set_constants(void)
{
    set_bits_double(&frame_delta_scale, 0x47ae147b, 0x3f847ae1);
    set_bits_float(&frame_delta_gain, 0x41100000);
    set_bits_float(&frame_delta_norm, 0x41200000);
    set_bits_double(&frame_delta_high, 0x9999999a, 0x3fc99999);
    set_bits_double(&frame_delta_low, 0x47ae147b, 0x3f947ae1);
    set_bits_double(&frame_phase_step, 0x33333333, 0x3fd33333);
    set_bits_float(&frame_phase_wrap, 0xbf800000);
    set_bits_float(&frame_phase_scale, 0x477fff00);
}

static unsigned int bits(float value)
{
    return *(unsigned int *)&value;
}

static void setup(unsigned int frames, float delta, float raw,
                  float phase, float timing)
{
    DAT_000007f9 = frames;
    DAT_00118e04 = delta;
    DAT_00118e08 = raw;
    DAT_00118e0c = phase;
    DAT_00118e10 = timing;
    DAT_00118e14 = 123.0f;
    DAT_0010cf08 = 0xdeadbeef;
    DAT_0002153c = 0xffffffff;
}

static int check(unsigned int old, unsigned int delta, unsigned int raw,
                 unsigned int phase, unsigned int timing,
                 unsigned int pointer, unsigned int word)
{
    return DAT_000007f9 == 0
        && bits(DAT_00118e14) == old
        && bits(DAT_00118e04) == delta
        && bits(DAT_00118e08) == raw
        && bits(DAT_00118e0c) == phase
        && bits(DAT_00118e10) == timing
        && DAT_0010cf08 == pointer
        && DAT_0002153c == word;
}

int main(void)
{
    set_constants();

    setup(100, 0.05f, 0.0f, 0.0f, 0.0f);
    frame_timer();
    put(check(0x3d4ccccd, 0x3e147ae1, 0x3f800000, 0x3d322d0e,
              0x3f800000, 0x8a48, 0x0b22) ? '0' : 'x');

    setup(1000, 1.0f, 0.0f, 0.95f, 0.0f);
    frame_timer();
    put(check(0x3f800000, 0x3e4ccccd, 0x41200000, 0x3c23d700,
              0x41200000, 0x8728, 0x028f) ? '1' : 'x');

    setup(2, 0.0f, 0.0f, 0.5f, 0.0f);
    frame_timer();
    put(check(0x00000000, 0x3ca3d70a, 0x3ca3d70a, 0x3f018937,
              0x3ca3d70a, 0xcf08, 0x8188) ? '2' : 'x');

    setup(3, 0.1f, 0.0f, 1.0f, 2.0f);
    frame_timer();
    put(check(0x3dcccccd, 0x3dbe76c9, 0x3cf5c28f, 0x3ce48e80,
              0x4001eb85, 0x9088, 0x0724) ? '3' : 'x');

    setup(50, 0.15f, 0.0f, 0.9f, 1.5f);
    frame_timer();
    put(check(0x3e19999a, 0x3e3d70a4, 0x3f000000, 0x3f749ba6,
              0x40000000, 0x88b8, 0xf49a) ? '4' : 'x');

    setup(50, 0.15f, 0.0f, 0.95f, 0.0f);
    frame_timer();
    put(check(0x3e19999a, 0x3e3d70a4, 0x3f000000, 0x3bb43900,
              0x3f000000, 0x88b8, 0x0168) ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
