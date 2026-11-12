/* The test harness for the rebuilt `sound_voice_finished` module.
 *
 * The harness holds the setup word `DAT_00008058`, the driver handle
 * `DAT_00018d94`, and the two driver stubs.  The field stub records the handle,
 * the voice index, and the call count, and returns a scripted word.  The idle
 * stub records the handle, the voice index, and the call count, and returns a
 * scripted result.
 *
 * The harness runs eight paths:
 *
 *     0  the setup word is zero: the function returns 0 and calls no stub
 *     1  the field equals the table id plus one and the voice is idle: the
 *        function returns 1
 *     2  the field equals the table id plus one and the voice is busy: the
 *        function returns 0
 *     3  the field differs from the table id plus one: the function returns 1
 *        and the idle stub does not run
 *     4  the table id is 0 and the field is 1: the function returns the idle
 *        result
 *     5  the table id is -1 and the field is 0: the function returns 0
 *     6  the field equals the table id: the function returns 1
 *     7  the table id 0x1234 and the status 0x1f: the two stubs receive the
 *        driver handle and the status
 *     D  the exit marker
 *
 * Every path checks the result, the stub call counts, and the recorded
 * arguments.
 */
#pragma off (check_stack)

int sound_voice_finished(int table_id, int status);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00008058;
int DAT_00018d94;

static char text[8];
static int field_calls;
static int field_handle;
static int field_voice;
static int field_result;
static int idle_calls;
static int idle_handle;
static int idle_voice;
static int idle_result;

int driver_voice_field(int handle, int voice)
{
    field_calls = field_calls + 1;
    field_handle = handle;
    field_voice = voice;
    return field_result;
}

int driver_voice_idle(int handle, int voice)
{
    idle_calls = idle_calls + 1;
    idle_handle = handle;
    idle_voice = voice;
    return idle_result;
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
    DAT_00008058 = 1;
    DAT_00018d94 = 0x23;
    field_calls = 0;
    field_handle = -1;
    field_voice = -1;
    field_result = 0;
    idle_calls = 0;
    idle_handle = -1;
    idle_voice = -1;
    idle_result = 0;
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
    DAT_00008058 = 0;
    result = sound_voice_finished(5, 3);
    ok = result == 0 && field_calls == 0 && idle_calls == 0;
    report(ok, '0');

    reset();
    field_result = 0x101;
    idle_result = 1;
    result = sound_voice_finished(0x100, 7);
    ok = result == 1 && field_calls == 1 && idle_calls == 1 &&
         field_handle == 0x23 && field_voice == 7 &&
         idle_handle == 0x23 && idle_voice == 7;
    report(ok, '1');

    reset();
    field_result = 0x201;
    idle_result = 0;
    result = sound_voice_finished(0x200, 4);
    ok = result == 0 && field_calls == 1 && idle_calls == 1 &&
         idle_voice == 4;
    report(ok, '2');

    reset();
    field_result = 0x302;
    idle_result = 1;
    result = sound_voice_finished(0x300, 4);
    ok = result == 1 && field_calls == 1 && idle_calls == 0;
    report(ok, '3');

    reset();
    field_result = 1;
    idle_result = 1;
    result = sound_voice_finished(0, 9);
    ok = result == 1 && field_calls == 1 && idle_calls == 1 &&
         field_voice == 9 && idle_voice == 9;
    report(ok, '4');

    reset();
    field_result = 0;
    idle_result = 0;
    result = sound_voice_finished(-1, 2);
    ok = result == 0 && field_calls == 1 && idle_calls == 1;
    report(ok, '5');

    reset();
    field_result = 0x400;
    idle_result = 1;
    result = sound_voice_finished(0x400, 1);
    ok = result == 1 && field_calls == 1 && idle_calls == 0;
    report(ok, '6');

    reset();
    field_result = 0x1235;
    idle_result = 1;
    result = sound_voice_finished(0x1234, 0x1f);
    ok = result == 1 && field_calls == 1 && idle_calls == 1 &&
         field_handle == 0x23 && field_voice == 0x1f &&
         idle_handle == 0x23 && idle_voice == 0x1f;
    report(ok, '7');

    put('D');
    quit();
    return 0;
}
