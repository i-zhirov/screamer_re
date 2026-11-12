/* The test harness for the rebuilt `gameport_pulse` module.
 *
 * The harness holds a scripted read sequence, a constant read mode, and the
 * `inportb` and `outportb` stubs.  The `inportb` stub returns the next script
 * byte, or the constant byte, and records the port and the call count.  The
 * `outportb` stub records the port, the value, and the call count.
 *
 * The harness runs these paths:
 *
 *     0  mask 1 on port 0x201 with the reads clear: count 0, out 0x00
 *     1  mask 1 with three settle reads and one count read: count 1
 *     2  mask 0x10 on port 0x203 with three count reads: count 3
 *     3  mask 1 with the constant read 0xff: the settle limit 0x1000 and the
 *        count limit 0x2000 give the count 0x2000 and the out byte 0xff
 *     4  mask 0x0101 on port 0x200 and a settle that clears at 0x02: the low
 *        byte selects the mask and the out byte is the last read 0x02
 *     5  mask 0x0100 clears the mask bits, because only `al` counts
 *     D  the exit marker
 *
 * Every path checks the count, the read and write counts, the port, and the
 * out byte.
 */
#include <i86.h>

#pragma off (check_stack)

unsigned int gameport_pulse(unsigned int mask, unsigned int port,
                            unsigned int selector);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char script[16];
static unsigned int script_len;
static unsigned int script_pos;
static unsigned char constant_byte;
static int constant_mode;
static unsigned short last_in_port;
static int in_calls;
static unsigned short last_out_port;
static unsigned char last_out_value;
static int out_calls;

unsigned char inportb(unsigned short port)
{
    unsigned char value;

    in_calls = in_calls + 1;
    last_in_port = port;
    if (constant_mode) {
        value = constant_byte;
    } else if (script_pos < script_len) {
        value = script[script_pos];
        script_pos = script_pos + 1;
    } else {
        value = 0;
    }
    return value;
}

void outportb(unsigned short port, unsigned char value)
{
    out_calls = out_calls + 1;
    last_out_port = port;
    last_out_value = value;
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

static void reset(unsigned int length)
{
    unsigned int index;

    script_len = length;
    script_pos = 0;
    constant_byte = 0;
    constant_mode = 0;
    last_in_port = 0xffff;
    in_calls = 0;
    last_out_port = 0xffff;
    last_out_value = 0xff;
    out_calls = 0;
    for (index = 0; index < 16; ++index) {
        script[index] = 0;
    }
}

static int check(unsigned int value, unsigned int expected, unsigned int port,
                 unsigned char out_value, int reads)
{
    return value == expected && out_calls == 1 && last_out_port == port
        && last_out_value == out_value && last_in_port == port
        && in_calls == reads;
}

int main(void)
{
    unsigned int value;

    reset(2);
    script[0] = 0x00;
    script[1] = 0x00;
    value = gameport_pulse(1, 0x201, 0x1234);
    put(check(value, 0, 0x201, 0x00, 2) ? '0' : 'x');

    reset(5);
    script[0] = 0x01;
    script[1] = 0x01;
    script[2] = 0x00;
    script[3] = 0x01;
    script[4] = 0x00;
    value = gameport_pulse(1, 0x201, 0);
    put(check(value, 1, 0x201, 0x00, 5) ? '1' : 'x');

    reset(5);
    script[0] = 0x00;
    script[1] = 0x10;
    script[2] = 0x10;
    script[3] = 0x10;
    script[4] = 0x00;
    value = gameport_pulse(0x10, 0x203, 0x55);
    put(check(value, 3, 0x203, 0x00, 5) ? '2' : 'x');

    reset(0);
    constant_mode = 1;
    constant_byte = 0xff;
    value = gameport_pulse(1, 0x201, 0);
    put(check(value, 0x2000, 0x201, 0xff, 0x3000) ? '3' : 'x');

    reset(3);
    script[0] = 0x03;
    script[1] = 0x02;
    script[2] = 0x00;
    value = gameport_pulse(0x0101, 0x200, 0x99);
    put(check(value, 0, 0x200, 0x02, 3) ? '4' : 'x');

    reset(1);
    script[0] = 0xff;
    value = gameport_pulse(0x0100, 0x201, 0);
    put(check(value, 0, 0x201, 0xff, 2) ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
