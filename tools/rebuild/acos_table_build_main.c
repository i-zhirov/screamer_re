/* The test harness for the rebuilt `acos_table_build` module.
 *
 * The harness holds the table pointer at `DAT_000c55f4` and the 513-entry
 * table.  The pointer starts in the middle of the table, because the function
 * indexes the table from -256 to 256.  The harness clears the table with a
 * sentinel before each call, so a missing store fails the check.
 *
 * The paths:
 *
 *     0  every entry equals the inverse cosine value from the same helper
 *     1  the entry at index 0 holds the middle angle 63
 *     2  the entry at index -256 holds 127 and the entry at index 256 holds 0
 *     3  the entry at index 128 holds 42
 *     4  the entry at index -128 holds 84
 *     5  the entry at index 64 holds 53
 *     6  the entry at index -192 holds 97
 *     D  the exit marker
 *
 * The expected value uses the same inverse cosine helper, the same scale, and
 * the same truncation, so the comparison follows the original arithmetic.
 *
 * The harness does not load the constant segment, so it sets the table pointer
 * and the two scale values at run time.  The scale is 40.42535442040344 from
 * the bit pattern 0x40443672037e6bc0, and the index scale is 256.0 from the
 * bit pattern 0x4070000000000000.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void acos_table_build(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

double acos(double value);
double acos_scale;
double acos_index_scale;
float acos_step;
float acos_start;

static char text[8];
static int table[513];
int *DAT_000c55f4;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_double(double *dest, unsigned int high, unsigned int low)
{
    union {
        double d;
        unsigned int u[2];
    } value;

    value.u[0] = low;
    value.u[1] = high;
    *dest = value.d;
}

static void set_float(float *dest, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *dest = value.f;
}

static void reset(void)
{
    int index;

    DAT_000c55f4 = table + 256;
    set_double(&acos_scale, 0x40443672u, 0x037e6bc0u);
    set_double(&acos_index_scale, 0x40700000u, 0x00000000u);
    set_float(&acos_step, 0x3b800000u);
    set_float(&acos_start, 0xbf800000u);
    for (index = 0; index < 513; index++) {
        table[index] = 0x55555555;
    }
}

static int expected(int slot)
{
    float value;
    volatile float angle;

    value = acos_start + (float)slot * acos_step;
    angle = (float)(acos((double)value) * acos_scale);
    return (int)angle;
}

static int verify_all(void)
{
    int slot;

    for (slot = 0; slot < 513; slot++) {
        if (table[slot] != expected(slot)) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    reset();
    acos_table_build();
    put(verify_all() ? '0' : 'x');
    put(table[256] == 63 ? '1' : 'x');
    put(table[0] == 127 && table[512] == 0 ? '2' : 'x');
    put(table[384] == 42 ? '3' : 'x');
    put(table[128] == 84 ? '4' : 'x');
    put(table[320] == 53 ? '5' : 'x');
    put(table[64] == 97 ? '6' : 'x');
    put('D');
    quit();
    return 0;
}
