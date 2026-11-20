/* The test harness for the rebuilt `sound_voice_pitch_set` module.
 *
 * The harness holds the setup word `DAT_00008058`, the driver handle
 * `DAT_00018d94`, the driver stub, and the print stub.  The driver stub records
 * the handle, the voice index, the pitch value, and the call count.  The print
 * stub records the message and the call count.
 *
 * The harness runs six paths:
 *
 *     0  the setup word is zero: the function stores nothing and prints nothing
 *     1  a zero pitch becomes 1 and the driver receives it
 *     2  a plain pitch passes to the driver unchanged
 *     3  the pitch 0x40000 stays at the boundary and prints nothing
 *     4  the pitch 0x40001 prints the message `PITCH`
 *     5  a negative pitch passes to the driver unchanged and prints nothing
 *     D  the exit marker
 *
 * Every path checks the call counts, the handle, the voice index, the pitch
 * value, and the printed message.
 */
#pragma off (check_stack)

void sound_voice_pitch_set(int voice, int value);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00008058;
int DAT_00018d94;

static char text[8];
static char pitch_message[] = "PITCH\n";

static int driver_calls;
static int driver_handle;
static int driver_voice;
static int driver_value;
static int print_calls;
static char *print_message;

int driver_voice_value_set(int handle, int voice, int value)
{
    driver_calls = driver_calls + 1;
    driver_handle = handle;
    driver_voice = voice;
    driver_value = value;
    return 0;
}

int printf(const char *format, ...)
{
    print_calls = print_calls + 1;
    print_message = (char *)format;
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

static int same(char *first, char *second)
{
    while (*first != 0) {
        if (*first != *second) {
            return 0;
        }
        ++first;
        ++second;
    }
    return *second == 0;
}

static void reset(void)
{
    DAT_00008058 = 1;
    DAT_00018d94 = 0x23;
    driver_calls = 0;
    driver_handle = -1;
    driver_voice = -1;
    driver_value = -1;
    print_calls = 0;
    print_message = 0;
}

static void run_case(int voice, int value, int expected_calls,
    int expected_voice, int expected_value, int expected_print,
    char marker)
{
    reset();
    sound_voice_pitch_set(voice, value);
    put(driver_calls == expected_calls
        && print_calls == expected_print
        && (expected_calls == 0
            || (driver_handle == DAT_00018d94
                && driver_voice == expected_voice
                && driver_value == expected_value))
        && (expected_print == 0 || same(print_message, pitch_message))
        ? marker : 'x');
}

int main(void)
{
    reset();
    DAT_00008058 = 0;
    sound_voice_pitch_set(5, 0x50000);
    put(driver_calls == 0 && print_calls == 0 ? '0' : 'x');

    run_case(5, 0, 1, 5, 1, 0, '1');
    run_case(7, 12345, 1, 7, 12345, 0, '2');
    run_case(1, 0x40000, 1, 1, 0x40000, 0, '3');
    run_case(2, 0x40001, 1, 2, 0x40001, 1, '4');
    run_case(3, -1, 1, 3, -1, 0, '5');

    put('D');
    quit();
    return 0;
}
