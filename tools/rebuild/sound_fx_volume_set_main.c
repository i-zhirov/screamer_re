/* The test harness for the rebuilt `sound_fx_volume_set` module.
 *
 * The harness holds the setup word `DAT_00008058`, the driver handle
 * `DAT_00018d94`, and the driver word store stub.  The stub records the
 * handle, the value, and the call count.
 *
 * The harness runs seven paths:
 *
 *     0  the setup word is zero: the function calls no stub
 *     1  the setup word is set and the value is 0: the stub sees 0
 *     2  the value is 0x7f: the stub sees 0x7f
 *     3  the value is 0x80: the stub sees 0x80
 *     4  the value is 0x123: the stub sees 0x123
 *     5  the value is 0x7fff: the stub sees 0x7fff
 *     6  two calls in a row record two values
 *     D  the exit marker
 *
 * Every path checks the call count, the handle, and the recorded value.
 */
#pragma off (check_stack)

void sound_fx_volume_set(int volume);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00008058;
int DAT_00018d94;

static char text[8];
static int store_calls;
static int store_handle;
static int store_value;

int driver_volume_store(int handle, int value)
{
    store_calls = store_calls + 1;
    store_handle = handle;
    store_value = value;
    return 0;
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
    store_calls = 0;
    store_handle = -1;
    store_value = -1;
}

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

int main(void)
{
    int ok;

    reset();
    DAT_00008058 = 0;
    sound_fx_volume_set(5);
    ok = store_calls == 0;
    report(ok, '0');

    reset();
    sound_fx_volume_set(0);
    ok = store_calls == 1 && store_handle == 0x23 && store_value == 0;
    report(ok, '1');

    reset();
    sound_fx_volume_set(0x7f);
    ok = store_calls == 1 && store_handle == 0x23 && store_value == 0x7f;
    report(ok, '2');

    reset();
    sound_fx_volume_set(0x80);
    ok = store_calls == 1 && store_handle == 0x23 && store_value == 0x80;
    report(ok, '3');

    reset();
    sound_fx_volume_set(0x123);
    ok = store_calls == 1 && store_handle == 0x23 && store_value == 0x123;
    report(ok, '4');

    reset();
    sound_fx_volume_set(0x7fff);
    ok = store_calls == 1 && store_handle == 0x23 && store_value == 0x7fff;
    report(ok, '5');

    reset();
    sound_fx_volume_set(0x40);
    sound_fx_volume_set(0x55);
    ok = store_calls == 2 && store_handle == 0x23 && store_value == 0x55;
    report(ok, '6');

    put('D');
    quit();
    return 0;
}
