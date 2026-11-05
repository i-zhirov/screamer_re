/* The test harness for the rebuilt `palette_build` module.
 *
 * The harness holds the 5000-entry table, the three float constants, and a
 * `pow` stub.  The module fills the table with the ramp values; the stub
 * records the first and the last argument pair and returns the base value, so
 * every table entry holds the ramp value `t` of its column.
 *
 * The harness checks:
 *
 *     0  the module called the stub 5000 times
 *     1  the first pair holds the base 0.5 and the exponent 50.0
 *     2  the last pair holds the base 0.995 and the exponent 1.0
 *     3  the table row 0, row 1, and row 49 hold the expected ramp values
 *     D  the exit marker
 *
 * The constants are set from their exact 32-bit and 64-bit patterns, and the
 * comparisons use the bit patterns too.  A float literal in this minimal image
 * becomes a read of the constant segment, which this harness does not load, so
 * a literal comparison would compare against zero.  The harness also keeps the
 * divisor 50 in a variable for the same reason.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void palette_build(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float DAT_001080e8[5000];
float palette_step;
double palette_base;
double palette_scale;

static char text[8];
static unsigned long calls;
static double first_base;
static double first_exponent;
static double last_base;
static double last_exponent;

double pow(double base, double exponent)
{
    if (calls == 0) {
        first_base = base;
        first_exponent = exponent;
    }
    last_base = base;
    last_exponent = exponent;
    calls = calls + 1;
    return base;
}

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

static int same_float(float a, float b)
{
    union {
        float f;
        unsigned int u;
    } x;
    union {
        float f;
        unsigned int u;
    } y;

    x.f = a;
    y.f = b;
    return x.u == y.u;
}

static int same_double(double a, double b)
{
    union {
        double d;
        unsigned int u[2];
    } x;
    union {
        double d;
        unsigned int u[2];
    } y;

    x.d = a;
    y.d = b;
    return x.u[0] == y.u[0] && x.u[1] == y.u[1];
}

static float ramp(int j)
{
    return (float)(j * palette_step + palette_base);
}

int main(void)
{
    unsigned long k;
    int fifty;

    fifty = 50;
    for (k = 0; k < 5000; k = k + 1) {
        DAT_001080e8[k] = -1.0f;
    }
    set_bits_float(&palette_step, 0x3ba3d70a);
    set_bits_double(&palette_base, 0x00000000, 0x3fe00000);
    set_bits_double(&palette_scale, 0x00000000, 0x40490000);
    calls = 0;
    palette_build();
    put(calls == 5000 ? '0' : 'x');
    put(same_float((float)first_base, ramp(0))
        && same_double(first_exponent, palette_scale) ? '1' : 'x');
    put(same_float((float)last_base, ramp(99))
        && same_double(last_exponent, first_exponent / fifty) ? '2' : 'x');
    put(same_float(DAT_001080e8[0], ramp(0))
        && same_float(DAT_001080e8[99], ramp(99))
        && same_float(DAT_001080e8[100], ramp(0))
        && same_float(DAT_001080e8[4900], ramp(0))
        && same_float(DAT_001080e8[4999], ramp(99)) ? '3' : 'x');
    put('D');
    quit();
    return 0;
}
