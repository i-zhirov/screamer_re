/* The test harness for the rebuilt `reciprocal_table_build` module.
 *
 * The harness holds the table pointer at `DAT_000c5da0` and the 700-entry
 * table.  It clears the table with a sentinel before each call, so a missing
 * store fails the check.
 *
 * The paths:
 *
 *     0  every entry from index 1 to 699 equals the reciprocal value
 *     1  the entry at index 1 holds 32767
 *     2  the entry at index 2 holds 16383
 *     3  the entry at index 3 holds 10922
 *     4  the entry at index 699 holds 46
 *     5  the entry at index 256 holds 127
 *     D  the exit marker
 *
 * The expected value uses the same scale and the same truncation, so the
 * comparison follows the original arithmetic.  The index 0 is skipped,
 * because the reciprocal of zero has no defined integer value.
 *
 * The harness does not load the constant segment, so it sets the table pointer
 * and the scale at run time.  The scale is 32767.0 from the bit pattern
 * 0x40dfffc000000000.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void reciprocal_table_build(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

double reciprocal_scale;

static char text[8];
static int table[700];
int *DAT_000c5da0;

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

static void reset(void)
{
    int index;

    DAT_000c5da0 = table;
    set_double(&reciprocal_scale, 0x40dfffc0u, 0x00000000u);
    for (index = 0; index < 700; index++) {
        table[index] = 0x55555555;
    }
}

static int expected(int index)
{
    return (int)((1.0 / (double)index) * reciprocal_scale);
}

static int verify_all(void)
{
    int index;

    for (index = 1; index < 700; index++) {
        if (table[index] != expected(index)) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    reset();
    reciprocal_table_build();
    put(verify_all() ? '0' : 'x');
    put(table[1] == 32767 ? '1' : 'x');
    put(table[2] == 16383 ? '2' : 'x');
    put(table[3] == 10922 ? '3' : 'x');
    put(table[699] == 46 ? '4' : 'x');
    put(table[256] == 127 ? '5' : 'x');
    put('D');
    quit();
    return 0;
}
