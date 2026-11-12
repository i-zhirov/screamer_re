/* The test harness for the rebuilt `music_select` module.
 *
 * The harness holds the music record and the two channel stubs.  The stubs
 * record their arguments and the call count.  The harness runs five paths:
 *
 *     0  a set success flag: free then start the channel, write the value
 *     1  a zero success flag: the same calls and the value -1
 *     2  a negative success flag: no call, the word at +0x0c keeps its value
 *     3  a second negative success flag -2: no call
 *     4  a set flag with the channel 0: the boundary index
 *     D  the exit marker
 */
#pragma off (check_stack)

void music_select(char *record, int value);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char record[0x14];
static int free_calls;
static int start_calls;
static int got_free;
static int got_start;
static int call_order;

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
    got_free = index;
    free_calls = free_calls + 1;
    call_order = call_order * 10 + 1;
}

void sound_channel_start(int index)
{
    got_start = index;
    start_calls = start_calls + 1;
    call_order = call_order * 10 + 2;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x14; ++index) {
        record[index] = 0;
    }
    free_calls = 0;
    start_calls = 0;
    got_free = -1;
    got_start = -1;
    call_order = 0;
}

int main(void)
{
    reset();
    *(int *)record = 3;
    *(int *)(record + 4) = 1;
    *(int *)(record + 0xc) = 0x55;
    music_select(record, 0x2a);
    put(free_calls == 1 && start_calls == 1 && got_free == 3
        && got_start == 3 && call_order == 12
        && *(int *)(record + 0xc) == 0x2a ? '0' : 'x');

    reset();
    *(int *)record = 5;
    *(int *)(record + 4) = 0;
    music_select(record, -1);
    put(free_calls == 1 && start_calls == 1 && got_free == 5
        && got_start == 5 && call_order == 12
        && *(int *)(record + 0xc) == -1 ? '1' : 'x');

    reset();
    *(int *)record = 7;
    *(int *)(record + 4) = -1;
    *(int *)(record + 0xc) = 0x55;
    music_select(record, 9);
    put(free_calls == 0 && start_calls == 0
        && *(int *)(record + 0xc) == 0x55 ? '2' : 'x');

    reset();
    *(int *)record = 7;
    *(int *)(record + 4) = -2;
    *(int *)(record + 0xc) = 0x66;
    music_select(record, 9);
    put(free_calls == 0 && start_calls == 0
        && *(int *)(record + 0xc) == 0x66 ? '3' : 'x');

    reset();
    *(int *)record = 0;
    *(int *)(record + 4) = 2;
    music_select(record, 0);
    put(free_calls == 1 && start_calls == 1 && got_free == 0
        && got_start == 0 && *(int *)(record + 0xc) == 0 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
