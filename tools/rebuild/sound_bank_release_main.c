/* The test harness for the rebuilt `sound_bank_release` module.
 *
 * The harness holds the setup word `DAT_00008058`, the archive context
 * `DAT_000062ac`, and the resource free stub.
 *
 * The stub records the call count, the context, and the name pointer.  The
 * harness runs three paths:
 *
 *     0  the zero setup word: the function makes no call
 *     1  the set setup word and the first context: one call for `SOUND_FX`
 *     2  another non-zero setup word and a sentinel context: one exact call
 *     D  the exit marker
 *
 * Every path checks the call count, the context, and the `SOUND_FX` name.
 */
#pragma off (check_stack)

void sound_bank_release(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00008058;
int DAT_000062ac;

static char text[8];

static int free_calls;
static int free_context;
static int free_name;

void resource_free_name(int context, char *name)
{
    free_calls = free_calls + 1;
    free_context = context;
    free_name = (int)name;
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

static int name_is(char *wanted)
{
    char *name = (char *)free_name;
    int index = 0;

    while (wanted[index] != 0) {
        if (name[index] != wanted[index]) {
            return 0;
        }
        index++;
    }
    return name[index] == 0;
}

static void reset(int gate, int context)
{
    DAT_00008058 = gate;
    DAT_000062ac = context;
    free_calls = 0;
    free_context = -1;
    free_name = 0;
}

static int sequence_is_expected(int calls, int context)
{
    if (free_calls != calls) {
        return 0;
    }
    if (calls == 0) {
        return 1;
    }
    if (free_context != context) {
        return 0;
    }
    if (!name_is("SOUND_FX")) {
        return 0;
    }
    return 1;
}

int main(void)
{
    reset(0, 0x11111111);
    sound_bank_release();
    put(sequence_is_expected(0, 0x11111111) ? '0' : 'x');

    reset(1, 0x22222222);
    sound_bank_release();
    put(sequence_is_expected(1, 0x22222222) ? '1' : 'x');

    reset(-1, -1);
    sound_bank_release();
    put(sequence_is_expected(1, -1) ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
