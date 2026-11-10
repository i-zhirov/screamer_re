/* The test harness for the rebuilt `mouse_release_wait` module.
 *
 * The harness holds the setup word `DAT_00008058`, the device handle
 * `DAT_00018d94`, and the phase counter `DAT_000007f9`.  The three mouse stubs
 * record the call count, the handle, and the slot index.  The phase stub
 * records the two arguments and returns the scripted delta.
 *
 * The harness runs five paths:
 *
 *     0  the setup word is zero: the drain is skipped and one wait step ends
 *        the function
 *     1  the setup word is set and one pressed slot releases at index 0
 *     2  the first slot is free and the second pressed slot releases at index 1
 *     3  the slot test returns 10 and no slot releases; three wait steps run
 *     4  three active counts release the slots 0 through 2
 *     D  the exit marker
 *
 * Every path checks the call counts, the last slot index, the saved phase, and
 * the handle of each stub call.
 */
#pragma off (check_stack)

void mouse_release_wait(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00008058;
int DAT_00018d94;
int DAT_000007f9;

static char text[8];

static int count_calls;
static int free_calls;
static int release_calls;
static int delta_calls;
static int count_script[8];
static int count_length;
static int count_pos;
static int free_script[8];
static int free_length;
static int free_pos;
static int delta_script[8];
static int delta_length;
static int delta_pos;
static int last_free_index;
static int last_release_index;
static int bad_handle;
static int delta_first_start;
static int delta_last_now;

int mouse_button_count(int handle)
{
    int value;

    count_calls = count_calls + 1;
    if (handle != DAT_00018d94) {
        bad_handle = 1;
    }
    if (count_pos < count_length) {
        value = count_script[count_pos];
    } else {
        value = 0;
    }
    count_pos = count_pos + 1;
    return value;
}

int mouse_button_free(int handle, int index)
{
    int value;

    free_calls = free_calls + 1;
    if (handle != DAT_00018d94) {
        bad_handle = 1;
    }
    last_free_index = index;
    if (free_pos < free_length) {
        value = free_script[free_pos];
    } else {
        value = 1;
    }
    free_pos = free_pos + 1;
    return value;
}

int mouse_button_release(int handle, int index)
{
    release_calls = release_calls + 1;
    if (handle != DAT_00018d94) {
        bad_handle = 1;
    }
    last_release_index = index;
    return 0;
}

int phase_delta(int start, int now)
{
    int value;

    if (delta_calls == 0) {
        delta_first_start = start;
    }
    delta_last_now = now;
    delta_calls = delta_calls + 1;
    if (delta_pos < delta_length) {
        value = delta_script[delta_pos];
    } else {
        value = 0x1e;
    }
    delta_pos = delta_pos + 1;
    return value;
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
    count_calls = 0;
    free_calls = 0;
    release_calls = 0;
    delta_calls = 0;
    count_pos = 0;
    free_pos = 0;
    delta_pos = 0;
    count_length = 0;
    free_length = 0;
    delta_length = 0;
    last_free_index = -1;
    last_release_index = -1;
    bad_handle = 0;
    delta_first_start = -1;
    delta_last_now = -1;
    DAT_00018d94 = 0x23;
    DAT_00008058 = 1;
    DAT_000007f9 = 0x4000;
}

int main(void)
{
    int ok;

    reset();
    DAT_00008058 = 0;
    delta_length = 1;
    delta_script[0] = 0x1e;
    mouse_release_wait();
    ok = count_calls == 0 && free_calls == 0 && release_calls == 0
        && delta_calls == 1 && delta_last_now == 0x4000 && bad_handle == 0;
    put(ok ? '0' : 'x');

    reset();
    count_length = 2;
    count_script[0] = 1;
    count_script[1] = 0;
    free_length = 1;
    free_script[0] = 0;
    delta_length = 2;
    delta_script[0] = 0x10005;
    delta_script[1] = 0x1001e;
    mouse_release_wait();
    ok = count_calls == 2 && free_calls == 1 && release_calls == 1
        && last_free_index == 0 && last_release_index == 0
        && delta_calls == 2 && delta_first_start == 0x4000
        && delta_last_now == 0x4000 && bad_handle == 0;
    put(ok ? '1' : 'x');

    reset();
    count_length = 3;
    count_script[0] = 1;
    count_script[1] = 1;
    count_script[2] = 0;
    free_length = 2;
    free_script[0] = 1;
    free_script[1] = 0;
    delta_length = 1;
    delta_script[0] = 0x1e;
    mouse_release_wait();
    ok = count_calls == 3 && free_calls == 2 && release_calls == 1
        && last_free_index == 1 && last_release_index == 1
        && delta_calls == 1 && delta_first_start == 0x4000 && bad_handle == 0;
    put(ok ? '2' : 'x');

    reset();
    count_length = 2;
    count_script[0] = 1;
    count_script[1] = 0;
    free_length = 1;
    free_script[0] = 10;
    delta_length = 3;
    delta_script[0] = 0;
    delta_script[1] = 0;
    delta_script[2] = 0x1e;
    mouse_release_wait();
    ok = count_calls == 2 && free_calls == 1 && release_calls == 0
        && last_free_index == 0 && delta_calls == 3
        && delta_first_start == 0x4000 && bad_handle == 0;
    put(ok ? '3' : 'x');

    reset();
    DAT_000007f9 = 0x5000;
    count_length = 4;
    count_script[0] = 3;
    count_script[1] = 2;
    count_script[2] = 1;
    count_script[3] = 0;
    free_length = 3;
    free_script[0] = 0;
    free_script[1] = 0;
    free_script[2] = 0;
    delta_length = 1;
    delta_script[0] = 0x1e;
    mouse_release_wait();
    ok = count_calls == 4 && free_calls == 3 && release_calls == 3
        && last_release_index == 2 && delta_calls == 1
        && delta_first_start == 0x5000 && bad_handle == 0;
    put(ok ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
