/* The test harness for the rebuilt `frame_update` module.
 *
 * The harness holds the key-state array, the three reset words, the frame
 * counter, the two input flags, the frame flag, the race context, the list
 * pointer, the music record, and the video buffer.  The ten callee stubs record
 * their arguments and count their calls.  The harness runs four paths:
 *
 *     0  the full path clears the key state, probes the memory, runs the menu
 *        calls, advances the counter, runs the input poll and the music call,
 *        and exits the frame loop after one turn
 *     1  the counter wrap path starts at 5, wraps to 0, and skips the input
 *        poll and the music call because the race context word is 0
 *     2  the input skip path holds the race context word and the flag
 *        `DAT_0011d2c0` but not `DAT_0011d2cc`, so the music call runs alone
 *     3  the loop path keeps the frame flag clear for three turns and then
 *        exits the loop
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void frame_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_000062ac;
char DAT_000062b0[16];
void *DAT_00002dec;
int DAT_0001fb08[20];
int DAT_00020988;
int DAT_00020880;
int DAT_000207f8;
int DAT_0011d2c0;
int DAT_0011d2cc;
int DAT_0011d234;
float DAT_0011d24c;
void *DAT_00121528;

static char text[8];
static char context[0x4c];
static char video_buffer[8];

static int memory_calls;
static void *memory_pool;
static unsigned int available;
static int menu_space_calls;
static int menu_frame_calls;
static int step_calls;
static int race_calls;
static int race_stop;
static int input_calls;
static int music_calls;
static char *music_record;
static int fill_calls;
static int memset_calls;
static void *memset_dest;
static unsigned int memset_count;
static int memset_value;
static int cleanup_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static unsigned int bits(float value)
{
    return *(unsigned int *)&value;
}

static int keys_clear(void)
{
    int index;

    for (index = 0; index < 20; ++index) {
        if (DAT_0001fb08[index] != 0) {
            return 0;
        }
    }
    return 1;
}

unsigned int memory_available(void *pool)
{
    memory_calls = memory_calls + 1;
    memory_pool = pool;
    return available;
}

void menu_space_init(void) { menu_space_calls = menu_space_calls + 1; }
void menu_frame(void) { menu_frame_calls = menu_frame_calls + 1; }
void frame_step(void) { step_calls = step_calls + 1; }

void race_frame(void)
{
    race_calls = race_calls + 1;
    if (race_stop != 0 && race_calls >= race_stop) {
        DAT_0011d234 = 1;
    }
}

int input_poll(void)
{
    input_calls = input_calls + 1;
    return 0;
}

int music_play(char *record)
{
    music_calls = music_calls + 1;
    music_record = record;
    return 0;
}

void video_fill(void) { fill_calls = fill_calls + 1; }

void race_cleanup(void) { cleanup_calls = cleanup_calls + 1; }

void *memset(void *dest, unsigned int count, int value)
{
    memset_calls = memset_calls + 1;
    memset_dest = dest;
    memset_count = count;
    memset_value = value;
    return dest;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 20; ++index) {
        DAT_0001fb08[index] = 0x11111111;
    }
    DAT_00020988 = 0x11111111;
    DAT_00020880 = 0x11111111;
    DAT_000207f8 = 0;
    DAT_0011d2c0 = 0;
    DAT_0011d2cc = 0;
    DAT_0011d234 = 0;
    DAT_0011d24c = 1.0f;
    DAT_000062ac = (void *)0x3333;
    DAT_00002dec = context;
    *(int *)(context + 0x48) = 0;
    DAT_00121528 = video_buffer;
    memory_calls = 0;
    memory_pool = 0;
    available = 0x12345678;
    menu_space_calls = 0;
    menu_frame_calls = 0;
    step_calls = 0;
    race_calls = 0;
    race_stop = 1;
    input_calls = 0;
    music_calls = 0;
    music_record = 0;
    fill_calls = 0;
    memset_calls = 0;
    memset_dest = 0;
    memset_count = 0;
    memset_value = -1;
    cleanup_calls = 0;
}

static int common(void)
{
    return keys_clear()
        && DAT_00020988 == 0
        && bits(DAT_0011d24c) == 0
        && DAT_00020880 == 0x12345678
        && memory_calls == 1
        && memory_pool == DAT_000062ac
        && menu_space_calls == 1
        && menu_frame_calls == 1
        && fill_calls == 2
        && memset_calls == 1
        && memset_dest == DAT_00121528
        && memset_count == 0x3e800
        && memset_value == 0
        && cleanup_calls == 1;
}

int main(void)
{
    reset();
    DAT_000207f8 = 2;
    DAT_0011d2c0 = 1;
    DAT_0011d2cc = 1;
    *(int *)(context + 0x48) = 1;
    race_stop = 1;
    frame_update();
    put(common() && DAT_000207f8 == 3 && step_calls == 1 && race_calls == 1
        && input_calls == 1 && music_calls == 1
        && music_record == DAT_000062b0 ? '0' : 'x');

    reset();
    DAT_000207f8 = 5;
    DAT_0011d2c0 = 1;
    DAT_0011d2cc = 1;
    *(int *)(context + 0x48) = 0;
    race_stop = 1;
    frame_update();
    put(common() && DAT_000207f8 == 0 && step_calls == 1 && race_calls == 1
        && input_calls == 0 && music_calls == 0 ? '1' : 'x');

    reset();
    DAT_000207f8 = 0;
    DAT_0011d2c0 = 1;
    DAT_0011d2cc = 0;
    *(int *)(context + 0x48) = 1;
    race_stop = 1;
    frame_update();
    put(common() && DAT_000207f8 == 1 && step_calls == 1 && race_calls == 1
        && input_calls == 0 && music_calls == 1
        && music_record == DAT_000062b0 ? '2' : 'x');

    reset();
    *(int *)(context + 0x48) = 0;
    race_stop = 3;
    frame_update();
    put(keys_clear() && DAT_00020988 == 0 && DAT_00020880 == 0x12345678
        && memory_calls == 1 && menu_space_calls == 1 && menu_frame_calls == 1
        && DAT_000207f8 == 1 && step_calls == 3 && race_calls == 3
        && input_calls == 0 && music_calls == 0 && fill_calls == 4
        && memset_calls == 1 && cleanup_calls == 1 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
