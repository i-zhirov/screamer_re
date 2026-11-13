/* The test harness for the rebuilt `video_bank_set` module.
 *
 * The harness holds the bank shift `DAT_0007c18c` and the `int386` stub.  The
 * stub records the interrupt number, the `ax` function, and the `dx` bank start
 * of every call.  The module passes the same register set as input and output,
 * so the stub also checks the two pointers.
 *
 * The paths:
 *
 *     0  shift 0, bank 0: dx 0x0000
 *     1  shift 0, bank 3: dx 0x0003
 *     2  shift 6, bank 1: dx 0x0040
 *     3  shift 6, bank 3: dx 0x00c0
 *     4  shift 4, bank 0x12: dx 0x0020
 *     5  shift 8, bank 3: the 8-bit result is 0
 *     6  shift 7, bank 0xff: dx 0x0080
 *     7  two calls with shift 5 and banks 3 and 4: two calls, the last dx 0x0080
 *     D  the exit marker
 *
 * Every path checks the interrupt number 0x10, the function 0x4f05, the bank
 * start, the call count, and the shared register pointer.
 */
#include <i86.h>

#pragma off (check_stack)

void video_bank_set(unsigned char bank);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0007c18c;
static char text[8];
static int call_count;
static int last_int;
static unsigned int last_ax;
static unsigned int last_dx;
static int shared_pointer;

int int386(int number, const union REGS *inregs, union REGS *outregs)
{
    call_count = call_count + 1;
    last_int = number;
    last_ax = inregs->w.ax;
    last_dx = inregs->w.dx;
    if (inregs != outregs) {
        shared_pointer = 0;
    }
    outregs->w.ax = 0x004f;
    return 0x4f;
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
    call_count = 0;
    last_int = 0;
    last_ax = 0;
    last_dx = 0;
    shared_pointer = 1;
}

static int check(int calls, unsigned int dx)
{
    return call_count == calls && last_int == 0x10 && last_ax == 0x4f05
        && last_dx == dx && shared_pointer;
}

int main(void)
{
    reset();
    DAT_0007c18c = 0;
    video_bank_set(0);
    put(check(1, 0x0000) ? '0' : 'x');

    reset();
    DAT_0007c18c = 0;
    video_bank_set(3);
    put(check(1, 0x0003) ? '1' : 'x');

    reset();
    DAT_0007c18c = 6;
    video_bank_set(1);
    put(check(1, 0x0040) ? '2' : 'x');

    reset();
    DAT_0007c18c = 6;
    video_bank_set(3);
    put(check(1, 0x00c0) ? '3' : 'x');

    reset();
    DAT_0007c18c = 4;
    video_bank_set(0x12);
    put(check(1, 0x0020) ? '4' : 'x');

    reset();
    DAT_0007c18c = 8;
    video_bank_set(3);
    put(check(1, 0x0000) ? '5' : 'x');

    reset();
    DAT_0007c18c = 7;
    video_bank_set(0xff);
    put(check(1, 0x0080) ? '6' : 'x');

    reset();
    DAT_0007c18c = 5;
    video_bank_set(3);
    video_bank_set(4);
    put(check(2, 0x0080) ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
