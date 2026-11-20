/* The test harness for the rebuilt `sound_frame_prepare` module.
 *
 * The harness holds the voice context and the four callee stubs.  The
 * `sound_voice_setup` stub records its call count and sets the voice count.
 * The two prepare stubs record the frame index and the source record of each
 * call.  The mix stub records the table and the count.
 *
 * The paths:
 *
 *     0  the first path with one voice: one source prepare call, the frame
 *        index 1, the source record advanced by 0x20, and the voice mix
 *     1  the first path with three voices and a preset frame index: the index
 *        resets to zero, the three calls see the indices 0, 1, and 2, and the
 *        source record advances by 0x60
 *     2  the second path with two voices: the near helper runs and the source
 *        record stays unchanged
 *     3  the second path with one voice: the voice record and the frame index
 *        follow one voice and the frame table base stays unchanged
 *     D  the exit marker
 */
#pragma off (check_stack)

#define NEAR_FLAG 0x40000000

struct sound_frame_context {
    int frame_base;             /* +0x00 */
    int voice_count;            /* +0x04 */
    int frame_index;            /* +0x08 */
    int voice_record;           /* +0x0c */
    int reserved_10[46];        /* +0x10 through +0xc4 */
    int source_record;          /* +0xc8 */
};

void sound_frame_prepare(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct sound_frame_context sound_voice_context;

static char text[8];
static int setup_calls;
static int setup_count;
static int prepare_calls;
static int prepare_index[8];
static int prepare_source[8];
static int near_calls;
static int near_index[8];
static int near_source[8];
static int mix_calls;
static void *mix_table;
static int mix_count;

void sound_voice_setup(void)
{
    setup_calls = setup_calls + 1;
    sound_voice_context.voice_count = setup_count;
}

void sound_source_frame_prepare(struct sound_frame_context *context)
{
    if (prepare_calls < 8) {
        prepare_index[prepare_calls] = context->frame_index;
        prepare_source[prepare_calls] = context->source_record;
    }
    prepare_calls = prepare_calls + 1;
}

void sub_8b0e8(struct sound_frame_context *context)
{
    if (near_calls < 8) {
        near_index[near_calls] = context->frame_index;
        near_source[near_calls] = context->source_record;
    }
    near_calls = near_calls + 1;
}

void sound_voice_mix(void *table, int count)
{
    mix_calls = mix_calls + 1;
    mix_table = table;
    mix_count = count;
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

static void reset(int count, int source, int index, int base)
{
    int position;

    for (position = 0; position < (int)sizeof(sound_voice_context); position++) {
        ((char *)&sound_voice_context)[position] = 0;
    }
    sound_voice_context.frame_base = base;
    sound_voice_context.frame_index = index;
    sound_voice_context.source_record = source;
    setup_calls = 0;
    setup_count = count;
    prepare_calls = 0;
    near_calls = 0;
    mix_calls = 0;
    mix_table = 0;
    mix_count = 0;
}

static int far_ok(int expected_calls, int source, int count)
{
    int index;

    if (setup_calls != 1 || prepare_calls != expected_calls
        || near_calls != 0 || mix_calls != 1) {
        return 0;
    }
    for (index = 0; index < expected_calls; index++) {
        if (prepare_index[index] != index) {
            return 0;
        }
        if (prepare_source[index] != source + index * 0x20) {
            return 0;
        }
    }
    return sound_voice_context.frame_index == expected_calls
        && sound_voice_context.voice_record == 0x7453c + expected_calls * 0x1f4
        && sound_voice_context.source_record == source + expected_calls * 0x20
        && mix_table == (void *)0x7a6e4
        && mix_count == count;
}

static int near_ok(int expected_calls, int source, int count, int base)
{
    int index;

    if (setup_calls != 1 || prepare_calls != 0
        || near_calls != expected_calls || mix_calls != 1) {
        return 0;
    }
    for (index = 0; index < expected_calls; index++) {
        if (near_index[index] != index || near_source[index] != source) {
            return 0;
        }
    }
    return sound_voice_context.frame_index == expected_calls
        && sound_voice_context.voice_record == 0x7453c + expected_calls * 0x1f4
        && sound_voice_context.source_record == source
        && sound_voice_context.frame_base == base
        && mix_table == (void *)0x7a6e4
        && mix_count == count;
}

int main(void)
{
    reset(1, 0x4000, 0, 0x1111);
    sound_frame_prepare();
    put(far_ok(1, 0x4000, 1) ? '0' : 'x');

    reset(3, 0x4000, 5, 0x2222);
    sound_frame_prepare();
    put(far_ok(3, 0x4000, 3) ? '1' : 'x');

    reset(2, 0x40000040, 0, 0x3333);
    sound_frame_prepare();
    put(near_ok(2, 0x40000040, 2, 0x3333) ? '2' : 'x');

    reset(1, 0x40000040, 7, 0x4444);
    sound_frame_prepare();
    put(near_ok(1, 0x40000040, 1, 0x4444) ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
