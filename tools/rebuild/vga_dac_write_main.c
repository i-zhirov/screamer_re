/* The test harness for the rebuilt `vga_dac_write` module.
 *
 * The harness holds the `outportb` stub.  The stub records the port and the
 * value of every write in order.
 *
 * The paths:
 *
 *     0  the ascending palette: the index write then 0x300 data bytes
 *     1  the descending palette: the data bytes follow the new source
 *     2  two calls in a row with the palette index times 3: two write pairs
 *     D  the exit marker
 *
 * Every path checks the write count, the port, and the value of every write.
 */
#include <i86.h>

#pragma off (check_stack)

void vga_dac_write(char *palette);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define DAC_BYTES 0x300
#define CALL_WRITES (DAC_BYTES + 1)
#define MAX_WRITES (2 * CALL_WRITES)

static char text[8];
static unsigned short out_ports[MAX_WRITES];
static unsigned char out_values[MAX_WRITES];
static int out_calls;
static unsigned char source[DAC_BYTES];

void outportb(unsigned short port, unsigned char value)
{
    if (out_calls < MAX_WRITES) {
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

static int check_call(int base)
{
    int index;

    if (out_ports[base] != 0x3c8 || out_values[base] != 0) {
        return 0;
    }
    for (index = 0; index < DAC_BYTES; ++index) {
        if (out_ports[base + 1 + index] != 0x3c9) {
            return 0;
        }
        if (out_values[base + 1 + index] != source[index]) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int index;

    for (index = 0; index < DAC_BYTES; ++index) {
        source[index] = (unsigned char)index;
    }
    out_calls = 0;
    vga_dac_write((char *)source);
    put(out_calls == CALL_WRITES && check_call(0) ? '0' : 'x');

    for (index = 0; index < DAC_BYTES; ++index) {
        source[index] = (unsigned char)(0xff - index);
    }
    out_calls = 0;
    vga_dac_write((char *)source);
    put(out_calls == CALL_WRITES && check_call(0) ? '1' : 'x');

    for (index = 0; index < DAC_BYTES; ++index) {
        source[index] = (unsigned char)(index * 3);
    }
    out_calls = 0;
    vga_dac_write((char *)source);
    vga_dac_write((char *)source);
    put(out_calls == MAX_WRITES && check_call(0) && check_call(CALL_WRITES)
        ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
