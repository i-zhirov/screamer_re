/* The test harness for the rebuilt `screen_setup_cleanup` module.
 *
 * The harness holds one stub for each of the seven callees.  Each stub appends
 * a letter to the trace and counts its calls.  The keyboard stub also records
 * the enable value, because the function must pass 0.
 *
 * The three paths:
 *
 *     0  the first call: the seven callees run in order with the value 0
 *     1  the second call: the same order and one more call per stub
 *     2  the third call: the same order and one more call per stub
 *     D  the exit marker
 *
 * Every path checks the order, the call counts, and the keyboard value.
 */
#pragma off (check_stack)

void screen_setup_cleanup(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char trace[16];
static int trace_length;
static int video_calls;
static int hook_calls;
static int hook_value;
static int materials_calls;
static int fonts_calls;
static int sprites_calls;
static int sound_calls;
static int clock_calls;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void add(char letter)
{
    if (trace_length < (int)sizeof(trace) - 1) {
        trace[trace_length] = letter;
        trace_length++;
        trace[trace_length] = 0;
    }
}

static void clear_trace(void)
{
    trace_length = 0;
    trace[0] = 0;
}

static void reset(void)
{
    clear_trace();
    video_calls = 0;
    hook_calls = 0;
    hook_value = -1;
    materials_calls = 0;
    fonts_calls = 0;
    sprites_calls = 0;
    sound_calls = 0;
    clock_calls = 0;
}

static int order_ok(void)
{
    if (trace_length != 7) {
        return 0;
    }
    if (trace[0] != 'A' || trace[1] != 'B' || trace[2] != 'C') {
        return 0;
    }
    if (trace[3] != 'D' || trace[4] != 'E' || trace[5] != 'F') {
        return 0;
    }
    return trace[6] == 'G';
}

static int counts_ok(int expected)
{
    if (video_calls != expected || hook_calls != expected) {
        return 0;
    }
    if (materials_calls != expected || fonts_calls != expected) {
        return 0;
    }
    if (sprites_calls != expected || sound_calls != expected) {
        return 0;
    }
    return clock_calls == expected;
}

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

void video_text_mode(void)
{
    video_calls = video_calls + 1;
    add('A');
}

void keyboard_hook(int enable)
{
    hook_calls = hook_calls + 1;
    hook_value = enable;
    add('B');
}

void materials_release(void)
{
    materials_calls = materials_calls + 1;
    add('C');
}

void fonts_release(void)
{
    fonts_calls = fonts_calls + 1;
    add('D');
}

void sprites_release(void)
{
    sprites_calls = sprites_calls + 1;
    add('E');
}

void sound_system_shutdown(void)
{
    sound_calls = sound_calls + 1;
    add('F');
}

void dos_datetime_set(void)
{
    clock_calls = clock_calls + 1;
    add('G');
}

int main(void)
{
    reset();
    screen_setup_cleanup();
    report(order_ok() && counts_ok(1) && hook_value == 0, '0');

    clear_trace();
    screen_setup_cleanup();
    report(order_ok() && counts_ok(2) && hook_value == 0, '1');

    clear_trace();
    screen_setup_cleanup();
    report(order_ok() && counts_ok(3) && hook_value == 0, '2');

    put('D');
    quit();
    return 0;
}
