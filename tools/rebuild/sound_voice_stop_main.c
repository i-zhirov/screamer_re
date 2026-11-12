/* The test harness for the rebuilt `sound_voice_stop` module.
 *
 * The harness holds the setup word `DAT_00008058`, the driver handle
 * `DAT_00018d94`, and the driver voice release stub.  The stub records the
 * handle, the voice index, and the call count, and returns a scripted result.
 *
 * The harness runs eight paths:
 *
 *     0  the setup word is zero: the function returns 0 and calls no stub
 *     1  the setup word is set and the voice is 3: the function returns -1
 *     2  the voice is 0 and the stub returns 1: the function returns -1
 *     3  the voice is -1: the function returns -1 and passes -1
 *     4  the voice is 0x1f: the function returns -1 and passes 0x1f
 *     5  the voice is 0x1000: the function returns -1 and passes 0x1000
 *     6  the stub returns -1: the function still returns -1
 *     7  two calls in a row record two handles and two voice indices
 *     D  the exit marker
 *
 * Every path checks the result, the stub call count, and the recorded
 * arguments.
 */
#pragma off (check_stack)

int sound_voice_stop(int voice);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00008058;
int DAT_00018d94;

static char text[8];
static int release_calls;
static int release_handle;
static int release_voice;
static int release_result;

int driver_voice_release(int handle, int voice)
{
    release_calls = release_calls + 1;
    release_handle = handle;
    release_voice = voice;
    return release_result;
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
    release_calls = 0;
    release_handle = -1;
    release_voice = -1;
    release_result = 0;
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
    result = sound_voice_stop(5);
    ok = result == 0 && release_calls == 0;
    report(ok, '0');

    reset();
    result = sound_voice_stop(3);
    ok = result == -1 && release_calls == 1 &&
         release_handle == 0x23 && release_voice == 3;
    report(ok, '1');

    reset();
    release_result = 1;
    result = sound_voice_stop(0);
    ok = result == -1 && release_calls == 1 &&
         release_handle == 0x23 && release_voice == 0;
    report(ok, '2');

    reset();
    result = sound_voice_stop(-1);
    ok = result == -1 && release_calls == 1 &&
         release_handle == 0x23 && release_voice == -1;
    report(ok, '3');

    reset();
    result = sound_voice_stop(0x1f);
    ok = result == -1 && release_calls == 1 &&
         release_handle == 0x23 && release_voice == 0x1f;
    report(ok, '4');

    reset();
    result = sound_voice_stop(0x1000);
    ok = result == -1 && release_calls == 1 &&
         release_handle == 0x23 && release_voice == 0x1000;
    report(ok, '5');

    reset();
    release_result = -1;
    result = sound_voice_stop(9);
    ok = result == -1 && release_calls == 1 &&
         release_handle == 0x23 && release_voice == 9;
    report(ok, '6');

    reset();
    result = sound_voice_stop(7);
    ok = result == -1 && release_calls == 1 &&
         release_handle == 0x23 && release_voice == 7;
    result = sound_voice_stop(9);
    ok = ok && result == -1 && release_calls == 2 &&
         release_handle == 0x23 && release_voice == 9;
    report(ok, '7');

    put('D');
    quit();
    return 0;
}
