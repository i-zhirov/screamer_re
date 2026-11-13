/* The test harness for the rebuilt `cd_system_shutdown` module.
 *
 * The harness holds the two callee stubs.  The stubs record the call order
 * and the argument.  The harness runs three paths:
 *
 *     0  one run: the function calls sound_driver_set with 0 and then
 *        dpmi_free_dos
 *     1  two runs: the sequence repeats
 *     2  three runs: the sequence repeats
 *     D  the exit marker
 *
 * The original has no branch and no return value, so every run checks the same
 * order and the same argument.
 */
#pragma off (check_stack)

void cd_system_shutdown(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define MAX_CALLS 16

enum {
    C_SOUND = 0,
    C_DPMI,
    C_BAD
};

static char text[8];

static int call_count;
static int call_id[MAX_CALLS];
static int call_value[MAX_CALLS];

static void record(int id, int value)
{
    if (call_count < MAX_CALLS) {
        call_id[call_count] = id;
        call_value[call_count] = value;
    }
    call_count++;
}

void sound_driver_set(int value)
{
    record(C_SOUND, value);
}

void dpmi_free_dos(void)
{
    record(C_DPMI, 0);
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

static int sequence_is_expected(int runs)
{
    int index;
    int base;

    if (call_count != runs * 2) {
        return 0;
    }
    for (index = 0; index < runs; index++) {
        base = index * 2;
        if (call_id[base] != C_SOUND) {
            return 0;
        }
        if (call_value[base] != 0) {
            return 0;
        }
        if (call_id[base + 1] != C_DPMI) {
            return 0;
        }
    }
    return 1;
}

static void reset_calls(void)
{
    call_count = 0;
}

int main(void)
{
    reset_calls();
    cd_system_shutdown();
    put(sequence_is_expected(1) ? '0' : 'x');

    reset_calls();
    cd_system_shutdown();
    cd_system_shutdown();
    put(sequence_is_expected(2) ? '1' : 'x');

    reset_calls();
    cd_system_shutdown();
    cd_system_shutdown();
    cd_system_shutdown();
    put(sequence_is_expected(3) ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
