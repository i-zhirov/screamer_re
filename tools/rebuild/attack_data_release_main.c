/* The test harness for the rebuilt `attack_data_release` module.
 *
 * The harness holds the archive context `DAT_000062ac`, the two attack replay
 * cells `DAT_0018ed0` and `DAT_0018eac`, and the resource free stub.
 *
 * The stub records the call count, the context, and the name pointer.  The
 * harness runs three paths:
 *
 *     0  the full sequence: the function frees `ATTACK` and clears both cells
 *     1  a second run with another context and other cell values
 *     2  a sentinel context and sentinel cells: the arguments stay exact
 *     D  the exit marker
 *
 * Every path checks the call count, the context, the `ATTACK` name, and the
 * two cleared cells.
 */
#pragma off (check_stack)

void attack_data_release(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_000062ac;
int DAT_0018ed0;
int DAT_0018eac;

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

static void reset(int context, int first, int second)
{
    DAT_000062ac = context;
    DAT_0018ed0 = first;
    DAT_0018eac = second;
    free_calls = 0;
    free_context = -1;
    free_name = 0;
}

static int sequence_is_expected(int context)
{
    if (free_calls != 1) {
        return 0;
    }
    if (free_context != context) {
        return 0;
    }
    if (!name_is("ATTACK")) {
        return 0;
    }
    if (DAT_0018ed0 != 0 || DAT_0018eac != 0) {
        return 0;
    }
    return 1;
}

int main(void)
{
    reset(0x11111111, 0x22222222, 0x33333333);
    attack_data_release();
    put(sequence_is_expected(0x11111111) ? '0' : 'x');

    reset(0x44444444, -1, 0x7fffffff);
    attack_data_release();
    put(sequence_is_expected(0x44444444) ? '1' : 'x');

    reset(-1, 0x55555555, -1);
    attack_data_release();
    put(sequence_is_expected(-1) ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
