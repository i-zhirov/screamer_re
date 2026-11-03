/* The test harness for the rebuilt `race_heading_update` module.
 *
 * The harness holds the four angles, the interpolation input, the frame step,
 * the three integer pairs, the six outputs, the flag, the four constants, and a
 * `pow` stub.  The stub records the first argument pair and returns the base,
 * so the power path is visible.
 *
 * The harness runs ten paths:
 *
 *     0  a zero interpolation value: the outputs follow the base angles and the
 *        integer pairs, and the early return leaves the inputs in place
 *     1  the power flag: the stub gets the base 0.5 and the exponent 2.0, and
 *        the factor 0.5 interpolates the three integer pairs
 *     2  the two headings at an exact 0x400 difference: both wrap by 0x800 and
 *        the interpolated heading follows the wrapped second heading
 *     3  the reverse 0x400 difference: only the second heading wraps
 *     4  a 0x3d1 difference: neither heading wraps, and a 2*pi angle masks to 0
 *     5  negative angles and a negative interpolation: the truncation keeps the
 *        heading masks and gives -1 for -1.5
 *     6  a negative frame step: the reset copies the new base angles and
 *        integer pairs and clears the interpolation value
 *     7  a positive frame step: only the interpolation value changes
 *     8  a negative zero interpolation value: the early return keeps it
 *     D  the exit marker
 *
 * The constants and the expected values are set from their exact bit patterns,
 * because a float literal in this minimal image reads the constant segment,
 * which this harness does not load.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void race_heading_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0011d240;
float DAT_00120388;
float DAT_00120394;
float DAT_00120384;
float DAT_0012038c;
float DAT_0012037c;
float DAT_00120380;
float DAT_00118e08;
int DAT_0012039c;
int DAT_001203a0;
int DAT_001203a4;
int DAT_001203a8;
int DAT_001203ac;
int DAT_001203b0;
int DAT_00109a0;
int DAT_00109a4;
int DAT_00109a8;
int DAT_00109ac;
int DAT_00109b0;
int DAT_00109b4;
float race_heading_scale;
float race_heading_pi;
double race_heading_power;
float race_heading_zero;

static char text[8];
static int pow_calls;
static double pow_base;
static double pow_exponent;

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

static unsigned int float_bits(float *p)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.f = *p;
    return v.u;
}

static unsigned int double_lo(double *p)
{
    union {
        double d;
        unsigned int u[2];
    } v;

    v.d = *p;
    return v.u[0];
}

static unsigned int double_hi(double *p)
{
    union {
        double d;
        unsigned int u[2];
    } v;

    v.d = *p;
    return v.u[1];
}

double pow(double base, double exponent)
{
    pow_calls = pow_calls + 1;
    pow_base = base;
    pow_exponent = exponent;
    return base;
}

static void reset(void)
{
    set_bits_float(&DAT_00120388, 0);
    set_bits_float(&DAT_00120394, 0);
    set_bits_float(&DAT_00120384, 0);
    set_bits_float(&DAT_0012038c, 0);
    set_bits_float(&DAT_0012037c, 0);
    set_bits_float(&DAT_00120380, 0);
    set_bits_float(&DAT_00118e08, 0);
    set_bits_float(&race_heading_scale, 0x44800000);
    set_bits_float(&race_heading_pi, 0x40490fdb);
    set_bits_double(&race_heading_power, 0, 0x40000000);
    set_bits_float(&race_heading_zero, 0);
    DAT_0011d240 = 0;
    DAT_0012039c = 0;
    DAT_001203a0 = 0;
    DAT_001203a4 = 0;
    DAT_001203a8 = 0;
    DAT_001203ac = 0;
    DAT_001203b0 = 0;
    DAT_00109a0 = -1;
    DAT_00109a4 = -1;
    DAT_00109a8 = -1;
    DAT_00109ac = -1;
    DAT_00109b0 = -1;
    DAT_00109b4 = -1;
    pow_calls = 0;
    pow_base = 0.0;
    pow_exponent = 0.0;
}

static void set_pairs(int first, int second, int third, int fourth,
    int fifth, int sixth)
{
    DAT_0012039c = first;
    DAT_001203a8 = second;
    DAT_001203a0 = third;
    DAT_001203ac = fourth;
    DAT_001203a4 = fifth;
    DAT_001203b0 = sixth;
}

int main(void)
{
    reset();
    set_bits_float(&DAT_00120384, 0x40000000);
    set_bits_float(&DAT_0012038c, 0x3f800000);
    set_pairs(1000, 200, 300, 500, 700, 900);
    race_heading_update();
    put(DAT_00109a0 == 651 && DAT_00109a8 == 325 && DAT_00109a4 == 0
        && DAT_00109ac == 200 && DAT_00109b0 == 500 && DAT_00109b4 == 900
        && pow_calls == 0 && float_bits(&DAT_0012037c) == 0 ? '0' : 'x');

    reset();
    DAT_0011d240 = 1;
    set_bits_float(&DAT_0012037c, 0x3f000000);
    set_bits_float(&DAT_00120380, 0x3f800000);
    set_bits_float(&DAT_00120384, 0x3f800000);
    set_bits_float(&DAT_0012038c, 0x40000000);
    set_pairs(1000, 200, 300, 500, 700, 900);
    race_heading_update();
    put(pow_calls == 1
        && double_lo(&pow_base) == 0 && double_hi(&pow_base) == 0x3fe00000
        && double_lo(&pow_exponent) == 0
        && double_hi(&pow_exponent) == 0x40000000
        && DAT_00109a0 == 325 && DAT_00109a8 == 651 && DAT_00109a4 == 0
        && DAT_00109ac == 600 && DAT_00109b0 == 400 && DAT_00109b4 == 800
        && float_bits(&DAT_0012037c) == 0x3f000000 ? '1' : 'x');

    reset();
    set_bits_float(&DAT_00120394, 0x40490fdb);
    race_heading_update();
    put(DAT_00109a4 == 3072 && pow_calls == 0
        && float_bits(&DAT_0012037c) == 0 ? '2' : 'x');

    reset();
    set_bits_float(&DAT_00120388, 0x40490fdb);
    race_heading_update();
    put(DAT_00109a4 == 2048 && pow_calls == 0 ? '3' : 'x');

    reset();
    set_bits_float(&DAT_00120384, 0x40c90fdb);
    set_bits_float(&DAT_00120394, 0x40400000);
    race_heading_update();
    put(DAT_00109a4 == 977 && DAT_00109a0 == 0 && pow_calls == 0
        ? '4' : 'x');

    reset();
    set_bits_float(&DAT_0012037c, 0x3f000000);
    set_bits_float(&DAT_00120384, 0xbf800000);
    set_bits_float(&DAT_0012038c, 0xc0000000);
    set_pairs(-3, 0, 0, 0, 0, 0);
    race_heading_update();
    put(DAT_00109a0 == 1723 && DAT_00109a8 == 1397 && DAT_00109ac == -1
        && DAT_00109a4 == 0 && float_bits(&DAT_0012037c) == 0x3f000000
        ? '5' : 'x');

    reset();
    set_bits_float(&DAT_0012037c, 0x3f000000);
    set_bits_float(&DAT_00120380, 0x3f800000);
    set_bits_float(&DAT_00118e08, 0x3f800000);
    set_bits_float(&DAT_00120394, 0x3f800000);
    set_pairs(111, 222, 333, 444, 555, 666);
    race_heading_update();
    put(float_bits(&DAT_00120388) == 0x3f800000
        && DAT_0012039c == 222 && DAT_001203a0 == 444
        && DAT_001203a4 == 666 && float_bits(&DAT_0012037c) == 0
        && DAT_00109ac == 166 && DAT_00109b0 == 388
        && DAT_00109b4 == 610 ? '6' : 'x');

    reset();
    set_bits_float(&DAT_0012037c, 0x3f800000);
    set_bits_float(&DAT_00120380, 0x3f800000);
    set_bits_float(&DAT_00118e08, 0x3e800000);
    set_bits_float(&DAT_00120394, 0x3f800000);
    set_pairs(111, 222, 333, 444, 555, 666);
    race_heading_update();
    put(float_bits(&DAT_0012037c) == 0x3f400000
        && float_bits(&DAT_00120388) == 0
        && DAT_0012039c == 111 && DAT_001203a0 == 333
        && DAT_001203a4 == 555 ? '7' : 'x');

    reset();
    DAT_0011d240 = 1;
    set_bits_float(&DAT_0012037c, 0x80000000);
    set_bits_float(&DAT_00120380, 0x3f800000);
    set_bits_float(&DAT_00118e08, 0x3f800000);
    set_bits_float(&DAT_00120394, 0x3f800000);
    set_pairs(111, 222, 333, 444, 555, 666);
    race_heading_update();
    put(float_bits(&DAT_0012037c) == 0x80000000 && pow_calls == 1
        && double_lo(&pow_base) == 0 && double_hi(&pow_base) == 0x80000000
        && float_bits(&DAT_00120388) == 0
        && DAT_0012039c == 111 && DAT_001203a0 == 333
        && DAT_001203a4 == 555 ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
