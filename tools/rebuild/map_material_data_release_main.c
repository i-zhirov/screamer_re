/* The test harness for the rebuilt `map_material_data_release` module.
 *
 * The harness holds the two callee stubs.  The stubs record the call order.
 * The harness runs three paths:
 *
 *     0  one run: the function calls the material release and then the map
 *        release
 *     1  two runs: the sequence repeats
 *     2  three runs: the sequence repeats
 *     D  the exit marker
 *
 * The original has no branch and no argument, so every run checks the same
 * order.  The harness checks that no other call happens.
 */
#pragma off (check_stack)

void map_material_data_release(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define MAX_CALLS 16

enum {
    C_MATERIAL = 0,
    C_MAP
};

static char text[8];

static int call_count;
static int call_id[MAX_CALLS];

void material_data_release(void)
{
    if (call_count < MAX_CALLS) {
        call_id[call_count] = C_MATERIAL;
    }
    call_count++;
}

void map_data_release(void)
{
    if (call_count < MAX_CALLS) {
        call_id[call_count] = C_MAP;
    }
    call_count++;
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

    if (call_count != runs * 2) {
        return 0;
    }
    for (index = 0; index < runs; index++) {
        if (call_id[index * 2] != C_MATERIAL) {
            return 0;
        }
        if (call_id[index * 2 + 1] != C_MAP) {
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
    map_material_data_release();
    put(sequence_is_expected(1) ? '0' : 'x');

    reset_calls();
    map_material_data_release();
    map_material_data_release();
    put(sequence_is_expected(2) ? '1' : 'x');

    reset_calls();
    map_material_data_release();
    map_material_data_release();
    map_material_data_release();
    put(sequence_is_expected(3) ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
