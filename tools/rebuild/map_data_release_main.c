/* The test harness for the rebuilt `map_data_release` module.
 *
 * The harness holds the archive head `DAT_0021574`, the map grid key
 * `DAT_0021468`, and the two callee stubs.
 *
 * The stubs record the call order and the arguments.  The harness runs three
 * paths:
 *
 *     0  the full sequence: the function frees `OBJ_LST`, frees `UNR_FILE`,
 *        and frees the map grid record, in that order, with the context and
 *        the key it receives
 *     1  a second run with another context and key: the sequence repeats and
 *        the arguments follow the two cells
 *     2  a sentinel context and key: the sequence repeats and the arguments
 *        stay exact
 *     D  the exit marker
 *
 * The original has no branch, so every path checks the same sequence.  The
 * harness checks the two resource names behind the free-name calls, because
 * the original passes the name pointer in `edx`.
 */
#pragma off (check_stack)

void map_data_release(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define MAX_CALLS 8
#define EXPECTED_CALLS 3

enum {
    C_FREE_NAME = 0,
    C_FIND
};

int DAT_0021574;
int DAT_0021468;
char map_obj_list[] = "OBJ_LST";
char map_unr_file[] = "UNR_FILE";

static char text[8];

static int call_count;
static int call_id[MAX_CALLS];
static int call_arg0[MAX_CALLS];
static int call_arg1[MAX_CALLS];

static void record(int id, int arg0, int arg1)
{
    int index;

    index = call_count;
    if (index < MAX_CALLS) {
        call_id[index] = id;
        call_arg0[index] = arg0;
        call_arg1[index] = arg1;
    }
    call_count = index + 1;
}

void resource_free_name(int context, char *name)
{
    record(C_FREE_NAME, context, (int)name);
}

void resource_find(int context, int key)
{
    record(C_FIND, context, key);
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

static int name_is(int address, char *wanted)
{
    char *name = (char *)address;
    int index = 0;

    while (wanted[index] != 0) {
        if (name[index] != wanted[index]) {
            return 0;
        }
        index++;
    }
    return name[index] == 0;
}

static int sequence_is_expected(int context, int key)
{
    if (call_count != EXPECTED_CALLS) {
        return 0;
    }
    if (call_id[0] != C_FREE_NAME || call_id[1] != C_FREE_NAME
        || call_id[2] != C_FIND) {
        return 0;
    }
    if (call_arg0[0] != context || call_arg0[1] != context
        || call_arg0[2] != context) {
        return 0;
    }
    if (!name_is(call_arg1[0], "OBJ_LST")) {
        return 0;
    }
    if (!name_is(call_arg1[1], "UNR_FILE")) {
        return 0;
    }
    if (call_arg1[2] != key) {
        return 0;
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
    DAT_0021574 = 0x11111111;
    DAT_0021468 = 0x22222222;
    map_data_release();
    put(sequence_is_expected(0x11111111, 0x22222222) ? '0' : 'x');

    reset_calls();
    DAT_0021574 = 0x33333333;
    DAT_0021468 = 0x44444444;
    map_data_release();
    put(sequence_is_expected(0x33333333, 0x44444444) ? '1' : 'x');

    reset_calls();
    DAT_0021574 = -1;
    DAT_0021468 = 0x7fffffff;
    map_data_release();
    put(sequence_is_expected(-1, 0x7fffffff) ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
