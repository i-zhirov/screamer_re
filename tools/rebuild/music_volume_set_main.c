/* The test harness for the rebuilt `music_volume_set` module.
 *
 * The harness holds one music bank record and the MIDI master volume stub.  The
 * stub records the forwarded volume and the call count.  The bank record has
 * the active flag at `+0x14`, the same offset as the original.
 *
 * The harness runs these paths:
 *
 *     0  a clear gate and the volume 0x7f: no call
 *     1  a set gate and the volume 0x7f: one call with 0x7f
 *     2  a set gate and the volume 0x80: one call with 0
 *     3  a set gate and the volume 0x123: one call with 0x23
 *     4  the gate 2 and the volume 0x40: one call with 0x40
 *     5  a negative gate and the volume 0x55: one call with 0x55
 *     D  the exit marker
 *
 * Every path checks the call count and, on a call, the forwarded volume.
 */
#include <i86.h>

#pragma off (check_stack)

struct music_bank {
    unsigned char pad[0x14];
    int active;
};

void music_volume_set(struct music_bank *bank, unsigned int volume);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static struct music_bank bank;
static unsigned int last_volume;
static int calls;

void midi_master_volume_set(unsigned int volume)
{
    calls = calls + 1;
    last_volume = volume;
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

static int check(int active, unsigned int volume, int want_calls,
                 unsigned int want_volume)
{
    bank.active = active;
    calls = 0;
    last_volume = 0xdeadbeefUL;
    music_volume_set(&bank, volume);
    if (calls != want_calls) {
        return 0;
    }
    if (want_calls == 0) {
        return 1;
    }
    return last_volume == want_volume;
}

int main(void)
{
    put(check(0, 0x7f, 0, 0) ? '0' : 'x');
    put(check(1, 0x7f, 1, 0x7f) ? '1' : 'x');
    put(check(1, 0x80, 1, 0x00) ? '2' : 'x');
    put(check(1, 0x123, 1, 0x23) ? '3' : 'x');
    put(check(2, 0x40, 1, 0x40) ? '4' : 'x');
    put(check(-1, 0x55, 1, 0x55) ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
