/* The test harness for the rebuilt `dpmi_free_dos` module.
 *
 * The harness holds the segment cell and the `dpmi_free_dos_block` stub.  The
 * stub records the call count and the segment of every call.
 *
 * The paths:
 *
 *     0  segment 0: no call and the cell keeps its value
 *     1  segment 0x1234: one call with 0x1234
 *     2  segment 0xffff: one call with 0xffff
 *     3  segment 0x0001: one call with 1
 *     4  two calls with 0x2000 and 0x3000: two calls in order
 *     D  the exit marker
 *
 * Every path checks the call count, the call segments, and the segment cell.
 */
#pragma off (check_stack)

void dpmi_free_dos(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned short DAT_001666a2;

#define MAX_CALLS 4
#define SENTINEL 0xDEAD

static char text[8];
static unsigned short call_segments[MAX_CALLS];
static int call_count;

void dpmi_free_dos_block(unsigned short segment)
{
    if (call_count < MAX_CALLS) {
        call_segments[call_count] = segment;
    }
    call_count = call_count + 1;
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

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static void reset(unsigned short segment)
{
    int index;

    DAT_001666a2 = segment;
    call_count = 0;
    for (index = 0; index < MAX_CALLS; index = index + 1) {
        call_segments[index] = SENTINEL;
    }
}

static int tail_ok(int used)
{
    int index;

    for (index = used; index < MAX_CALLS; index = index + 1) {
        if (call_segments[index] != SENTINEL) {
            return 0;
        }
    }
    return 1;
}

static int calls_are(int count, unsigned short first, unsigned short second)
{
    if (call_count != count) {
        return 0;
    }
    if (count >= 1 && call_segments[0] != first) {
        return 0;
    }
    if (count >= 2 && call_segments[1] != second) {
        return 0;
    }
    return tail_ok(count);
}

int main(void)
{
    reset(0);
    dpmi_free_dos();
    check('0', calls_are(0, 0, 0) && DAT_001666a2 == 0);

    reset(0x1234);
    dpmi_free_dos();
    check('1', calls_are(1, 0x1234, 0) && DAT_001666a2 == 0x1234);

    reset(0xffff);
    dpmi_free_dos();
    check('2', calls_are(1, 0xffff, 0) && DAT_001666a2 == 0xffff);

    reset(0x0001);
    dpmi_free_dos();
    check('3', calls_are(1, 0x0001, 0) && DAT_001666a2 == 0x0001);

    reset(0x2000);
    dpmi_free_dos();
    DAT_001666a2 = 0x3000;
    dpmi_free_dos();
    check('4', calls_are(2, 0x2000, 0x3000) && DAT_001666a2 == 0x3000);

    put('D');
    quit();
    return 0;
}
