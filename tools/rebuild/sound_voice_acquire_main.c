/* The test harness for the rebuilt `sound_voice_acquire` module.
 *
 * The harness holds the driver handle `DAT_00018d94`, the driver field stub,
 * and the table lookup stub.  The field stub records the handle, the voice
 * index, and the call count, and returns a scripted word.  The lookup stub
 * records the sample, the call count, and returns a scripted voice.
 *
 * The harness runs these paths:
 *
 *     0  the cached voice is -1: the driver stub does not run, and the
 *        function returns the lookup result
 *     1  the cached voice keeps the sample: the function returns the voice
 *        and calls no lookup
 *     2  the cached voice holds another sample: the function returns the
 *        lookup result
 *     3  the cached voice is 0 and keeps the sample 0: the function returns 0
 *     4  a large sample matches the driver word: the function returns the
 *        cached voice
 *     5  the driver word differs from the sample 0: the function returns the
 *        lookup result
 *     6  the voice 0x1f keeps the sample: the two stubs receive the driver
 *        handle and the voice
 *     7  the cached voice is -1 and the sample is 0: the lookup receives 0
 *     D  the exit marker
 *
 * Every path checks the result, the stub call counts, and the recorded
 * arguments.
 */
#pragma off (check_stack)

int sound_voice_acquire(int sample, int voice);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00018d94;

static char text[8];
static int field_calls;
static int field_handle;
static int field_voice;
static int field_result;
static int lookup_calls;
static int lookup_sample;
static int lookup_result;

int driver_voice_sample(int handle, int voice)
{
    field_calls = field_calls + 1;
    field_handle = handle;
    field_voice = voice;
    return field_result;
}

int table_lookup(int sample)
{
    lookup_calls = lookup_calls + 1;
    lookup_sample = sample;
    return lookup_result;
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

static void reset(void)
{
    DAT_00018d94 = 0x23;
    field_calls = 0;
    field_handle = -1;
    field_voice = -1;
    field_result = 0;
    lookup_calls = 0;
    lookup_sample = -1;
    lookup_result = 0;
}

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

int main(void)
{
    int result;
    int ok;

    reset();
    field_result = 0;
    lookup_result = 0x11;
    result = sound_voice_acquire(9, -1);
    ok = result == 0x11 && field_calls == 0 && lookup_calls == 1 &&
         lookup_sample == 9;
    report(ok, '0');

    reset();
    field_result = 4;
    lookup_result = 0x12;
    result = sound_voice_acquire(4, 5);
    ok = result == 5 && field_calls == 1 && lookup_calls == 0 &&
         field_handle == 0x23 && field_voice == 5;
    report(ok, '1');

    reset();
    field_result = 5;
    lookup_result = 0x13;
    result = sound_voice_acquire(4, 7);
    ok = result == 0x13 && field_calls == 1 && lookup_calls == 1 &&
         field_voice == 7 && lookup_sample == 4;
    report(ok, '2');

    reset();
    field_result = 0;
    lookup_result = 0x14;
    result = sound_voice_acquire(0, 0);
    ok = result == 0 && field_calls == 1 && lookup_calls == 0 &&
         field_voice == 0;
    report(ok, '3');

    reset();
    field_result = 0x1234;
    lookup_result = 0x15;
    result = sound_voice_acquire(0x1234, 3);
    ok = result == 3 && field_calls == 1 && lookup_calls == 0 &&
         field_voice == 3;
    report(ok, '4');

    reset();
    field_result = 1;
    lookup_result = 0x20;
    result = sound_voice_acquire(0, 2);
    ok = result == 0x20 && field_calls == 1 && lookup_calls == 1 &&
         field_voice == 2 && lookup_sample == 0;
    report(ok, '5');

    reset();
    field_result = 4;
    lookup_result = 0x16;
    result = sound_voice_acquire(4, 0x1f);
    ok = result == 0x1f && field_calls == 1 && lookup_calls == 0 &&
         field_handle == 0x23 && field_voice == 0x1f;
    report(ok, '6');

    reset();
    field_result = 0;
    lookup_result = 0;
    result = sound_voice_acquire(0, -1);
    ok = result == 0 && field_calls == 0 && lookup_calls == 1 &&
         lookup_sample == 0;
    report(ok, '7');

    put('D');
    quit();
    return 0;
}
