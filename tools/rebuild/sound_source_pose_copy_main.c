/* The test harness for the rebuilt `sound_source_pose_copy` module.
 *
 * The harness holds the global state pointer at 0x74468, the sample offset at
 * 0x74528, a state block with the table offset at `+0x0c`, a frame table, and
 * a voice record.  The source pointer forms as table + offset + 0x1c + sample
 * offset, so each path selects its source block by the two offsets.
 *
 * The harness runs four paths:
 *
 *     0  a positive table offset and sample offset select the 0x4c block
 *     1  the zero offsets select the 0x1c block and copy -1
 *     2  a negative sample offset selects the 0x1c block with the extremes
 *     3  a negative table offset and a positive sample offset select 0x3c
 *     D  the exit marker
 *
 * Every path checks all 24 sign-extended dwords in the two voice blocks.
 */
#pragma off (check_stack)

void sound_source_pose_copy(char *voice, char *table);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STATE_OFFSET 0x0c
#define VOICE_FIRST 0x28
#define VOICE_SECOND 0x58
#define POSE_ELEMENTS 12

void *sound_state;
int sound_sample_offset;

static char text[4];
static char state[0x40];
static char table[0x400];
static char voice[0x100];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    print(text);
}

static void clear_bytes(char *base, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        base[index] = 0;
    }
}

static void set_int(char *base, int offset, int value)
{
    *(int *)(base + offset) = value;
}

static int get_int(char *base, int offset)
{
    return *(int *)(base + offset);
}

static void set_short(char *base, int offset, short value)
{
    *(short *)(base + offset) = value;
}

static void reset(void)
{
    clear_bytes(state, 0x40);
    clear_bytes(table, 0x400);
    clear_bytes(voice, 0x100);
    sound_state = state;
    sound_sample_offset = 0;
}

static void fill_source(int offset, int first)
{
    int index;

    for (index = 0; index < POSE_ELEMENTS * 2; index++) {
        set_short(table, offset + index * 2, (short)(first + index * 7));
    }
}

static int check_source(int offset, int first)
{
    int index;

    for (index = 0; index < POSE_ELEMENTS; index++) {
        if (get_int(voice, VOICE_FIRST + index * 4)
            != (int)(short)(first + index * 7)) {
            return 0;
        }
    }
    for (index = 0; index < POSE_ELEMENTS; index++) {
        if (get_int(voice, VOICE_SECOND + index * 4)
            != (int)(short)(first + (POSE_ELEMENTS + index) * 7)) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int ok;
    int index;

    reset();
    set_int(state, STATE_OFFSET, 0x20);
    sound_sample_offset = 0x10;
    fill_source(0x4c, -40);
    sound_source_pose_copy(voice, table);
    ok = check_source(0x4c, -40);
    put(ok ? '0' : 'x');

    reset();
    set_int(state, STATE_OFFSET, 0);
    sound_sample_offset = 0;
    for (index = 0; index < POSE_ELEMENTS * 2; index++) {
        set_short(table, 0x1c + index * 2, -1);
    }
    sound_source_pose_copy(voice, table);
    ok = 1;
    for (index = 0; index < POSE_ELEMENTS; index++) {
        if (get_int(voice, VOICE_FIRST + index * 4) != -1) {
            ok = 0;
        }
        if (get_int(voice, VOICE_SECOND + index * 4) != -1) {
            ok = 0;
        }
    }
    put(ok ? '1' : 'x');

    reset();
    set_int(state, STATE_OFFSET, 0x10);
    sound_sample_offset = -0x10;
    for (index = 0; index < POSE_ELEMENTS * 2; index++) {
        set_short(table, 0x1c + index * 2,
            (short)((index & 1) ? -32768 : 32767));
    }
    sound_source_pose_copy(voice, table);
    ok = 1;
    for (index = 0; index < POSE_ELEMENTS * 2; index++) {
        int expected = (index & 1) ? -32768 : 32767;
        int value = (index < POSE_ELEMENTS)
            ? get_int(voice, VOICE_FIRST + index * 4)
            : get_int(voice, VOICE_SECOND + (index - POSE_ELEMENTS) * 4);
        if (value != expected) {
            ok = 0;
        }
    }
    put(ok ? '2' : 'x');

    reset();
    set_int(state, STATE_OFFSET, -0x20);
    sound_sample_offset = 0x40;
    fill_source(0x3c, 100);
    sound_source_pose_copy(voice, table);
    ok = check_source(0x3c, 100);
    put(ok ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
