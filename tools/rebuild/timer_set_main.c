/* The test harness for the rebuilt `timer_set` module.
 *
 * The harness holds the `outportb` stub.  The stub records the port and the
 * value of every write in order.
 *
 * The paths:
 *
 *     0  divisor 0: mode 0x36 at 0x43, then 0x00 and 0x00 at 0x40
 *     1  divisor 0x1234: the low byte 0x34 and the high byte 0x12
 *     2  divisor 0xffff: the low byte 0xff and the high byte 0xff
 *     3  divisor 0x00ff: the low byte 0xff and the high byte 0x00
 *     4  divisor 0xff00: the low byte 0x00 and the high byte 0xff
 *     5  two calls with the divisors 0x0001 and 0x0203: six writes, the last
 *        three from the second call
 *     D  the exit marker
 *
 * Every path checks the write count, the port, and the value of every write.
 */
#include <i86.h>

#pragma off (check_stack)

void timer_set(unsigned int divisor);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned short out_ports[8];
static unsigned char out_values[8];
static int out_calls;

void outportb(unsigned short port, unsigned char value)
{
    if (out_calls < 8) {
        out_ports[out_calls] = port;
        out_values[out_calls] = value;
    }
    out_calls = out_calls + 1;
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
    int index;

    out_calls = 0;
    for (index = 0; index < 8; ++index) {
        out_ports[index] = 0xffff;
        out_values[index] = 0xff;
    }
}

static int check(int base, unsigned char low, unsigned char high)
{
    return out_ports[base] == 0x43 && out_values[base] == 0x36
        && out_ports[base + 1] == 0x40 && out_values[base + 1] == low
        && out_ports[base + 2] == 0x40 && out_values[base + 2] == high;
}

int main(void)
{
    reset();
    timer_set(0);
    put(out_calls == 3 && check(0, 0x00, 0x00) ? '0' : 'x');

    reset();
    timer_set(0x1234);
    put(out_calls == 3 && check(0, 0x34, 0x12) ? '1' : 'x');

    reset();
    timer_set(0xffff);
    put(out_calls == 3 && check(0, 0xff, 0xff) ? '2' : 'x');

    reset();
    timer_set(0x00ff);
    put(out_calls == 3 && check(0, 0xff, 0x00) ? '3' : 'x');

    reset();
    timer_set(0xff00);
    put(out_calls == 3 && check(0, 0x00, 0xff) ? '4' : 'x');

    reset();
    timer_set(0x0001);
    timer_set(0x0203);
    put((out_calls == 6 && check(0, 0x01, 0x00) && check(3, 0x03, 0x02))
        ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
