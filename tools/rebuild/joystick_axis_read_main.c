/* The test harness for the rebuilt `joystick_axis_read` module.
 *
 * The harness holds the scripted pulse count, the scripted button byte, and
 * one stub for each of the two callees.  The `gameport_pulse` stub records the
 * mask, the port, and the selector and returns the scripted count.  The
 * `inportb` stub records the port and returns the scripted button byte.
 *
 * The harness runs these paths:
 *
 *     0  mask 1 on port 0x201, count 0x123 and buttons 0x10: the result is
 *        0x10123 and the selector is 2
 *     1  mask 2 on port 0x201, count 0xfff and buttons 0xf0: the result is
 *        0xf0fff and the selector is 4
 *     2  mask 8 on port 0x201, count 0x2000 and buttons 0: the timing masks to
 *        0 and the selector is 0x0a
 *     3  mask 4 on port 0x203, count 0xabc and buttons 0x50: the result is
 *        0x50abc and the selector is 0x0a
 *     4  mask 1 on port 0x200, count 1 and buttons 0x20: the result is 0x20001
 *        and the selector has the low bit cleared to 0
 *     D  the exit marker
 *
 * Every path checks the result, the call count, the forwarded arguments, and
 * the port of the byte read.
 */
#include <i86.h>

#pragma off (check_stack)

unsigned int joystick_axis_read(unsigned int mask, unsigned int port);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned int script_count;
static unsigned char script_buttons;
static unsigned int last_mask;
static unsigned int last_port;
static unsigned int last_selector;
static unsigned int last_in_port;
static int pulse_calls;
static int in_calls;

unsigned int gameport_pulse(unsigned int mask, unsigned int port,
                            unsigned int selector)
{
    pulse_calls = pulse_calls + 1;
    last_mask = mask;
    last_port = port;
    last_selector = selector;
    return script_count;
}

unsigned char inportb(unsigned short port)
{
    in_calls = in_calls + 1;
    last_in_port = port;
    return script_buttons;
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

static int check(unsigned int value, unsigned int expected, unsigned int mask,
                 unsigned int port, unsigned int selector)
{
    return pulse_calls == 1 && in_calls == 1
        && last_mask == mask && last_port == port
        && last_selector == selector && last_in_port == port
        && value == expected;
}

static void reset(unsigned int count, unsigned char buttons)
{
    script_count = count;
    script_buttons = buttons;
    last_mask = 0xffffffffUL;
    last_port = 0xffffffffUL;
    last_selector = 0xffffffffUL;
    last_in_port = 0xffffffffUL;
    pulse_calls = 0;
    in_calls = 0;
}

int main(void)
{
    unsigned int value;

    reset(0x123UL, 0x10);
    value = joystick_axis_read(1, 0x201);
    put(check(value, 0x10123UL, 1, 0x201, 2) ? '0' : 'x');

    reset(0xfffUL, 0xf0);
    value = joystick_axis_read(2, 0x201);
    put(check(value, 0xf0fffUL, 2, 0x201, 4) ? '1' : 'x');

    reset(0x2000UL, 0x00);
    value = joystick_axis_read(8, 0x201);
    put(check(value, 0x00000UL, 8, 0x201, 0x0a) ? '2' : 'x');

    reset(0xabcUL, 0x50);
    value = joystick_axis_read(4, 0x203);
    put(check(value, 0x50abcUL, 4, 0x203, 0x0a) ? '3' : 'x');

    reset(1, 0x20);
    value = joystick_axis_read(1, 0x200);
    put(check(value, 0x20001UL, 1, 0x200, 0) ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
