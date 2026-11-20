/* The test harness for the rebuilt `palette_set` module.
 *
 * The harness holds the source palette and the two helper stubs.
 *
 * The retrace stub appends `R` to the event order.  The DAC stub appends `D`,
 * records the pointer, and copies the 0x300 bytes.  The harness runs three
 * paths:
 *
 *     0  the ascending palette: one retrace, one DAC write, the same pointer
 *     1  the descending palette: the copy follows the new source bytes
 *     2  two calls in a row: two retrace waits and two DAC writes in order
 *     D  the exit marker
 *
 * Every path checks the call counts, the event order, the pointer, and the
 * copied bytes.
 */
#pragma off (check_stack)

void palette_set(char *palette);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char source[0x300];
static unsigned char got[0x300];
static int retrace_calls;
static int dac_calls;
static int order_index;
static char order[8];
static char *dac_pointer;

void vga_retrace_wait(void)
{
    retrace_calls = retrace_calls + 1;
    order[order_index] = 'R';
    order_index = order_index + 1;
}

void vga_dac_write(char *palette)
{
    int index;

    dac_calls = dac_calls + 1;
    order[order_index] = 'D';
    order_index = order_index + 1;
    dac_pointer = palette;
    for (index = 0; index < 0x300; ++index) {
        got[index] = (unsigned char)palette[index];
    }
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

static void reset(void)
{
    retrace_calls = 0;
    dac_calls = 0;
    order_index = 0;
    order[0] = 0;
    dac_pointer = 0;
}

static int copied_is_source(void)
{
    int index;

    for (index = 0; index < 0x300; ++index) {
        if (got[index] != source[index]) {
            return 0;
        }
    }
    return 1;
}

static int order_is(int pairs)
{
    int index;

    for (index = 0; index < pairs; ++index) {
        if (order[index * 2] != 'R' || order[index * 2 + 1] != 'D') {
            return 0;
        }
    }
    return order_index == pairs * 2;
}

int main(void)
{
    int index;

    for (index = 0; index < 0x300; ++index) {
        source[index] = (unsigned char)index;
    }
    reset();
    palette_set((char *)source);
    put(retrace_calls == 1 && dac_calls == 1
        && dac_pointer == (char *)source && order_is(1)
        && copied_is_source() ? '0' : 'x');

    for (index = 0; index < 0x300; ++index) {
        source[index] = (unsigned char)(0xff - index);
    }
    reset();
    palette_set((char *)source);
    put(retrace_calls == 1 && dac_calls == 1
        && dac_pointer == (char *)source && order_is(1)
        && copied_is_source() ? '1' : 'x');

    for (index = 0; index < 0x300; ++index) {
        source[index] = (unsigned char)(index * 3);
    }
    got[0] = 0x5a;
    reset();
    palette_set((char *)source);
    palette_set((char *)source);
    put(retrace_calls == 2 && dac_calls == 2
        && dac_pointer == (char *)source && order_is(2)
        && copied_is_source() ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
