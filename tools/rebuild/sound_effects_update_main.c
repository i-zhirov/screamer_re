/* The test harness for the rebuilt `sound_effects_update` module.
 *
 * The harness holds the globals of the module and the ten stubs.  The sequence
 * helper sets the active count, and the frame helper sets the voice count.  The
 * event helper reads the frame index and returns the configured results.  Each
 * stub appends its letter to the call sequence.  The harness runs five paths:
 *
 *     0  the zero active count returns after the timer, sequence, and source
 *        calls
 *     1  the zero voice count returns after the mix prepare
 *     2  two frames with two voices and one event per frame: the index resets,
 *        the event path runs twice, and the loop globals get their values
 *     3  one frame with three voices and the events at the indexes 0 and 2:
 *        the middle voice skips the event path
 *     4  one frame with two voices and no event: the event globals keep their
 *        sentinel values
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void sound_effects_update(int driver);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int sound_effect_handle;
int sound_active_count;
int sound_voice_count;
int sound_frame_repeat;
int sound_voice_index;
int sound_voice_remaining;
void *sound_song_data;
int sound_sample_table;
int sound_volume_left;
int sound_volume_right;

static char text[8];
static char sequence[64];
static int sequence_length;
static char expected_0[4];
static char expected_1[5];
static char expected_2[18];
static char expected_3[16];
static char expected_4[9];

static int timer_calls;
static int sequence_calls;
static int source_calls;
static int mix_prepare_calls;
static int voice_mix_calls;
static void *voice_mix_table;
static int voice_mix_count;
static int frame_prepare_calls;
static int event_next_calls;
static int event_transform_calls;
static int event_render_calls;
static int event_convert_calls;
static void *event_convert_source;

static int sequence_active_result;
static int frame_remaining;
static int next_results[8];
static int next_result_count;
static int next_result_index;
static int seen_index[8];
static int seen_count;

static void build_expected(void)
{
    expected_0[0] = 'A'; expected_0[1] = 'B'; expected_0[2] = 'C';
    expected_0[3] = 0;
    expected_1[0] = 'A'; expected_1[1] = 'B'; expected_1[2] = 'C';
    expected_1[3] = 'D'; expected_1[4] = 0;
    expected_2[0] = 'A'; expected_2[1] = 'B'; expected_2[2] = 'C';
    expected_2[3] = 'D'; expected_2[4] = 'E'; expected_2[5] = 'F';
    expected_2[6] = 'G'; expected_2[7] = 'H'; expected_2[8] = 'I';
    expected_2[9] = 'J'; expected_2[10] = 'G'; expected_2[11] = 'F';
    expected_2[12] = 'G'; expected_2[13] = 'H'; expected_2[14] = 'I';
    expected_2[15] = 'J'; expected_2[16] = 'G'; expected_2[17] = 0;
    expected_3[0] = 'A'; expected_3[1] = 'B'; expected_3[2] = 'C';
    expected_3[3] = 'D'; expected_3[4] = 'E'; expected_3[5] = 'F';
    expected_3[6] = 'G'; expected_3[7] = 'H'; expected_3[8] = 'I';
    expected_3[9] = 'J'; expected_3[10] = 'G'; expected_3[11] = 'G';
    expected_3[12] = 'H'; expected_3[13] = 'I'; expected_3[14] = 'J';
    expected_3[15] = 0;
    expected_4[0] = 'A'; expected_4[1] = 'B'; expected_4[2] = 'C';
    expected_4[3] = 'D'; expected_4[4] = 'E'; expected_4[5] = 'F';
    expected_4[6] = 'G'; expected_4[7] = 'G'; expected_4[8] = 0;
}

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void sequence_add(char letter)
{
    if (sequence_length < 63) {
        sequence[sequence_length] = letter;
        sequence_length = sequence_length + 1;
        sequence[sequence_length] = 0;
    }
}

static int same_sequence(char *expected)
{
    int index;

    for (index = 0; expected[index] != 0; index++) {
        if (sequence[index] != expected[index]) {
            return 0;
        }
    }
    return sequence[index] == 0;
}

void sound_timer_update(void)
{
    sequence_add('A');
    timer_calls = timer_calls + 1;
}

void sound_sequence_prepare(void)
{
    sequence_add('B');
    sequence_calls = sequence_calls + 1;
    sound_active_count = sequence_active_result;
}

void sound_source_update(void)
{
    sequence_add('C');
    source_calls = source_calls + 1;
}

void sound_mix_prepare(void)
{
    sequence_add('D');
    mix_prepare_calls = mix_prepare_calls + 1;
}

void sound_voice_mix(void *table, int count)
{
    sequence_add('E');
    voice_mix_calls = voice_mix_calls + 1;
    voice_mix_table = table;
    voice_mix_count = count;
}

void sound_frame_prepare(void)
{
    sequence_add('F');
    frame_prepare_calls = frame_prepare_calls + 1;
    sound_voice_remaining = frame_remaining;
}

int sound_event_next(void)
{
    int result;

    sequence_add('G');
    event_next_calls = event_next_calls + 1;
    if (seen_count < 8) {
        seen_index[seen_count] = sound_voice_index;
        seen_count = seen_count + 1;
    }
    result = 0;
    if (next_result_index < next_result_count) {
        result = next_results[next_result_index];
        next_result_index = next_result_index + 1;
    }
    return result;
}

void sound_event_transform(void)
{
    sequence_add('H');
    event_transform_calls = event_transform_calls + 1;
}

void sound_event_render(void)
{
    sequence_add('I');
    event_render_calls = event_render_calls + 1;
}

void sound_event_convert(void *source)
{
    sequence_add('J');
    event_convert_calls = event_convert_calls + 1;
    event_convert_source = source;
}

static void reset(void)
{
    int index;

    sequence_length = 0;
    sequence[0] = 0;
    timer_calls = 0;
    sequence_calls = 0;
    source_calls = 0;
    mix_prepare_calls = 0;
    voice_mix_calls = 0;
    voice_mix_table = 0;
    voice_mix_count = 0;
    frame_prepare_calls = 0;
    event_next_calls = 0;
    event_transform_calls = 0;
    event_render_calls = 0;
    event_convert_calls = 0;
    event_convert_source = 0;
    sequence_active_result = 0;
    frame_remaining = 0;
    next_result_count = 0;
    next_result_index = 0;
    seen_count = 0;
    for (index = 0; index < 8; index++) {
        next_results[index] = 0;
        seen_index[index] = -1;
    }
    sound_effect_handle = 0x11111111;
    sound_active_count = 0x22222222;
    sound_voice_count = 0x33333333;
    sound_frame_repeat = 0x44444444;
    sound_voice_index = 0x55555555;
    sound_voice_remaining = 0x66666666;
    sound_song_data = (void *)0x77777777;
    sound_sample_table = 0x12121212;
    sound_volume_left = 0x13131313;
    sound_volume_right = 0x14141414;
}

static void set_events(int count)
{
    int index;

    next_result_count = count;
    next_result_index = 0;
    for (index = 0; index < count; index++) {
        next_results[index] = 0;
    }
}

static int guarded_ok(void)
{
    return timer_calls == 1 && sequence_calls == 1 && source_calls == 1
        && sound_effect_handle == 0x6350;
}

int main(void)
{
    build_expected();
    reset();
    sequence_active_result = 0;
    sound_voice_count = 5;
    sound_effects_update(0x6350);
    put(guarded_ok() && same_sequence(expected_0) && mix_prepare_calls == 0
        && voice_mix_calls == 0 && frame_prepare_calls == 0
        && event_next_calls == 0
        && sound_song_data == (void *)0x77777777 ? '0' : 'x');

    reset();
    sequence_active_result = 1;
    sound_voice_count = 0;
    sound_effects_update(0x6350);
    put(guarded_ok() && same_sequence(expected_1) && mix_prepare_calls == 1
        && voice_mix_calls == 0 && frame_prepare_calls == 0
        && sound_active_count == 1 && sound_song_data == (void *)0x77777777
        ? '1' : 'x');

    reset();
    sequence_active_result = 1;
    sound_voice_count = 2;
    frame_remaining = 2;
    set_events(4);
    next_results[0] = 1;
    next_results[1] = 0;
    next_results[2] = 1;
    next_results[3] = 0;
    sound_effects_update(0x6350);
    put(guarded_ok() && same_sequence(expected_2)
        && mix_prepare_calls == 1 && voice_mix_calls == 1
        && voice_mix_table == (void *)0x45790 && voice_mix_count == 2
        && frame_prepare_calls == 2 && event_next_calls == 4
        && event_transform_calls == 2 && event_render_calls == 2
        && event_convert_calls == 2
        && event_convert_source == (void *)0x4d3c8
        && seen_count == 4 && seen_index[0] == 0 && seen_index[1] == 1
        && seen_index[2] == 0 && seen_index[3] == 1
        && sound_song_data == (void *)0x45790
        && sound_frame_repeat == 0 && sound_voice_index == 2
        && sound_voice_remaining == 0
        && sound_sample_table == 0x694f0
        && sound_volume_left == 0x17700000
        && sound_volume_right == 0x17700000 ? '2' : 'x');

    reset();
    sequence_active_result = 1;
    sound_voice_count = 1;
    frame_remaining = 3;
    set_events(3);
    next_results[0] = 1;
    next_results[1] = 0;
    next_results[2] = 1;
    sound_effects_update(0x6350);
    put(guarded_ok() && same_sequence(expected_3)
        && mix_prepare_calls == 1 && voice_mix_calls == 1
        && frame_prepare_calls == 1 && event_next_calls == 3
        && event_transform_calls == 2 && event_render_calls == 2
        && event_convert_calls == 2
        && seen_count == 3 && seen_index[0] == 0 && seen_index[1] == 1
        && seen_index[2] == 2
        && sound_frame_repeat == 0 && sound_voice_index == 3
        && sound_voice_remaining == 0 ? '3' : 'x');

    reset();
    sequence_active_result = 1;
    sound_voice_count = 1;
    frame_remaining = 2;
    set_events(2);
    sound_effects_update(0x6350);
    put(guarded_ok() && same_sequence(expected_4)
        && mix_prepare_calls == 1 && voice_mix_calls == 1
        && frame_prepare_calls == 1 && event_next_calls == 2
        && event_transform_calls == 0 && event_render_calls == 0
        && event_convert_calls == 0
        && seen_count == 2 && seen_index[0] == 0 && seen_index[1] == 1
        && sound_song_data == (void *)0x45790
        && sound_frame_repeat == 0 && sound_voice_index == 2
        && sound_voice_remaining == 0
        && sound_sample_table == 0x12121212
        && sound_volume_left == 0x13131313
        && sound_volume_right == 0x14141414 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
