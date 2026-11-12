/* The test harness for the rebuilt `music_voice_stop` module.
 *
 * The harness holds one music bank record and the channel free stub.  The stub
 * records the forwarded index and the call count.  The record has the channel
 * index at `+0x00` and the success flag at `+0x04`, the same offsets as the
 * original.
 *
 * The harness runs these paths:
 *
 *     0  a set success flag and the channel 3: one call with 3
 *     1  a zero success flag and the channel 5: one call with 5
 *     2  the success flag -1: no call
 *     3  the success flag -2: no call
 *     4  a set success flag and the channel 0: one call with 0
 *     D  the exit marker
 *
 * Every path checks the call count and, on a call, the forwarded index.
 */
#pragma off (check_stack)

struct music_bank {
    int channel;
    int success;
};

void music_voice_stop(struct music_bank *bank);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static struct music_bank bank;
static int free_calls;
static int got_index;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void sound_channel_free(int index)
{
    got_index = index;
    free_calls = free_calls + 1;
}

static void reset(void)
{
    bank.channel = 0;
    bank.success = 0;
    free_calls = 0;
    got_index = -1;
}

int main(void)
{
    reset();
    bank.channel = 3;
    bank.success = 1;
    music_voice_stop(&bank);
    put(free_calls == 1 && got_index == 3 ? '0' : 'x');

    reset();
    bank.channel = 5;
    bank.success = 0;
    music_voice_stop(&bank);
    put(free_calls == 1 && got_index == 5 ? '1' : 'x');

    reset();
    bank.channel = 7;
    bank.success = -1;
    music_voice_stop(&bank);
    put(free_calls == 0 ? '2' : 'x');

    reset();
    bank.channel = 9;
    bank.success = -2;
    music_voice_stop(&bank);
    put(free_calls == 0 ? '3' : 'x');

    reset();
    bank.channel = 0;
    bank.success = 2;
    music_voice_stop(&bank);
    put(free_calls == 1 && got_index == 0 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
