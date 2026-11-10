/* The test harness for the rebuilt `video_selector_setup` module.
 *
 * The harness holds the table base `DAT_0007bcfa`, the `fs` selector dword
 * `DAT_00021490`, the three output globals `DAT_00021480`, `DAT_0007bcf0`, and
 * `DAT_0007abd3`, the source dword `DAT_000214ec`, the copy target
 * `DAT_00073664`, and the six selector patch stubs.
 *
 * The stubs record the call order.  The harness runs three paths:
 *
 *     0  base 0x10000, selector 0x20, value 0x11111111: the function stores the
 *        base, forms the descriptor 0x10022, calls the six stubs in order, and
 *        copies the value to the two targets
 *     1  base 0x20000, selector 0x1f, value 0x22222222: the selector rounds
 *        down to 0x18, so the descriptor is 0x2001a
 *     2  base 0x30000, selector 0, value 0x33333333: the descriptor is 0x30002
 *     D  the exit marker
 *
 * Every path checks the call count, the call order, the base store, the
 * descriptor, and the two copies.
 */
#pragma off (check_stack)

void video_selector_setup(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define MAX_CALLS 8
#define EXPECTED_CALLS 6

unsigned long DAT_0007bcfa;
unsigned long DAT_00021490;
unsigned long DAT_00021480;
unsigned long DAT_0007bcf0;
unsigned long DAT_000214ec;
unsigned long DAT_00073664;
unsigned long DAT_0007abd3;

static char text[8];

static int call_count;
static int call_id[MAX_CALLS];

static void record(int id)
{
    int index;

    index = call_count;
    if (index < MAX_CALLS) {
        call_id[index] = id;
    }
    call_count = index + 1;
}

void selector_patch_a(void)
{
    record(0);
}

void selector_patch_b(void)
{
    record(1);
}

void selector_patch_c(void)
{
    record(2);
}

void selector_patch_d(void)
{
    record(3);
}

void selector_patch_e(void)
{
    record(4);
}

void selector_patch_f(void)
{
    record(5);
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

static int setup_is_expected(
    unsigned long base, unsigned long selector, unsigned long value
)
{
    unsigned long offset;
    int index;

    offset = selector >> 3;
    offset = offset << 3;
    if (call_count != EXPECTED_CALLS) {
        return 0;
    }
    for (index = 0; index < EXPECTED_CALLS; index++) {
        if (call_id[index] != index) {
            return 0;
        }
    }
    if (DAT_00021480 != base) {
        return 0;
    }
    if (DAT_0007bcf0 != base + offset + 2) {
        return 0;
    }
    if (DAT_00073664 != value) {
        return 0;
    }
    if (DAT_0007abd3 != value) {
        return 0;
    }
    return 1;
}

static void reset(void)
{
    call_count = 0;
    DAT_00021480 = 0xaaaaaaaaUL;
    DAT_0007bcf0 = 0xbbbbbbbbUL;
    DAT_00073664 = 0xccccccccUL;
    DAT_0007abd3 = 0xddddddddUL;
}

int main(void)
{
    reset();
    DAT_0007bcfa = 0x00010000UL;
    DAT_00021490 = 0x20UL;
    DAT_000214ec = 0x11111111UL;
    video_selector_setup();
    put(setup_is_expected(0x00010000UL, 0x20UL, 0x11111111UL) ? '0' : 'x');

    reset();
    DAT_0007bcfa = 0x00020000UL;
    DAT_00021490 = 0x1fUL;
    DAT_000214ec = 0x22222222UL;
    video_selector_setup();
    put(setup_is_expected(0x00020000UL, 0x1fUL, 0x22222222UL) ? '1' : 'x');

    reset();
    DAT_0007bcfa = 0x00030000UL;
    DAT_00021490 = 0x00UL;
    DAT_000214ec = 0x33333333UL;
    video_selector_setup();
    put(setup_is_expected(0x00030000UL, 0x00UL, 0x33333333UL) ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
