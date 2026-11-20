/* The test harness for the rebuilt `vga_retrace_wait` module.
 *
 * The harness holds a scripted read sequence and the `inportb` stub.  The stub
 * returns the next script byte and records the port and the read count.
 *
 * The paths:
 *
 *     0  bit 3 set, then clear, then clear, then set: four reads
 *     1  bit 3 clear, then set: two reads
 *     2  three sets, one clear, two clears, then one set: seven reads
 *     D  the exit marker
 *
 * Every path checks the read count and the port of every read.
 */
#include <i86.h>

#pragma off (check_stack)

void vga_retrace_wait(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define MAX_READS 16

static char text[8];
static unsigned char script[MAX_READS];
static unsigned int script_len;
static unsigned int script_pos;
static unsigned short in_ports[MAX_READS];
static int in_calls;

unsigned char inportb(unsigned short port)
{
    unsigned char value;

    if (in_calls < MAX_READS) {
        in_ports[in_calls] = port;
    }
    in_calls = in_calls + 1;
    if (script_pos < script_len) {
        value = script[script_pos];
        script_pos = script_pos + 1;
    } else {
        value = 0;
    }
    return value;
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
    in_calls = 0;
    for (index = 0; index < MAX_READS; ++index) {
        script[index] = 0;
        in_ports[index] = 0xffff;
    }
}

static int port_is_3da(int reads)
{
    int index;

    for (index = 0; index < reads; ++index) {
        if (in_ports[index] != 0x3da) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    reset(4);
    script[0] = 0x08;
    script[1] = 0x00;
    script[2] = 0x00;
    script[3] = 0x08;
    vga_retrace_wait();
    put(in_calls == 4 && port_is_3da(4) ? '0' : 'x');

    reset(2);
    script[0] = 0x00;
    script[1] = 0x08;
    vga_retrace_wait();
    put(in_calls == 2 && port_is_3da(2) ? '1' : 'x');

    reset(7);
    script[0] = 0x08;
    script[1] = 0x08;
    script[2] = 0x08;
    script[3] = 0x00;
    script[4] = 0x00;
    script[5] = 0x00;
    script[6] = 0x08;
    vga_retrace_wait();
    put(in_calls == 7 && port_is_3da(7) ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
