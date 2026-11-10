/* The test harness for the rebuilt `object_list_release_all` module.
 *
 * The harness holds the 0x78-record object table at 0x1c bytes, the list at
 * 0x6350, and one callee stub.  The stub records the table, the list, and the
 * entry index, and returns a scripted value for each call.  The harness does
 * not model the list contents, because the function only passes the list to the
 * helper and never reads it.
 *
 * The harness runs nine paths:
 *
 *     0  an empty table: the release helper is not called and the call returns
 *        0
 *     1  one active record: the helper receives the table, the list, and the
 *        index 0, and the call returns 0
 *     2  records 0 and 2 are active and record 1 is free: the release runs at
 *        index 0 and index 2 in table order
 *     3  record 5 is active: the helper receives the index 5
 *     4  the release returns 0x0c at the first active record: the call returns
 *        0x0c at once and the later active record stays untouched
 *     5  the release returns 0 at index 0 and 7 at index 3: the call returns 7
 *        after the two releases
 *     6  the last record 0x77 is active: the release runs at index 0x77
 *     7  the file field holds 0x100: a non-zero, non-one file record is active
 *     D  the exit marker
 *
 * The table, the list, and the script use BSS state, because the minimal image
 * does not load the initialized data page.
 */
#pragma off (check_stack)

#define RECORD_COUNT 0x78
#define RECORD_SIZE 0x1c
#define CALL_LIMIT 16

struct object_record {
    char name[0x0c];
    void *block;
    void *chunks;
    void *file;
    int slot;
};

int object_list_release_all(void *table, int *list);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int object_slot_release(void *table, int *list, int *index);

static unsigned char table[RECORD_COUNT * RECORD_SIZE];
static int list[16];
static char text[8];

static int release_calls;
static void *release_table;
static void *release_list;
static int release_index[CALL_LIMIT];
static int release_result[CALL_LIMIT];

int object_slot_release(void *table, int *list, int *index)
{
    int call = release_calls;

    if (call < CALL_LIMIT) {
        release_index[call] = *index;
    }
    release_calls++;
    release_table = table;
    release_list = list;
    if (call < CALL_LIMIT) {
        return release_result[call];
    }
    return 0;
}

static struct object_record *record_at(int index)
{
    return (struct object_record *)(table + index * RECORD_SIZE);
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

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

static void reset(void)
{
    int index;

    for (index = 0; index < RECORD_COUNT * RECORD_SIZE; index++) {
        table[index] = 0;
    }
    for (index = 0; index < 16; index++) {
        list[index] = 0;
    }
    for (index = 0; index < CALL_LIMIT; index++) {
        release_index[index] = 0;
        release_result[index] = 0;
    }
    release_calls = 0;
    release_table = 0;
    release_list = 0;
}

static void set_active(int index, int value)
{
    record_at(index)->file = (void *)value;
}

int main(void)
{
    int result;
    int ok;

    reset();
    result = object_list_release_all(table, list);
    ok = result == 0 && release_calls == 0;
    report(ok, '0');

    reset();
    set_active(0, 1);
    result = object_list_release_all(table, list);
    ok = result == 0
        && release_calls == 1 && release_index[0] == 0
        && release_table == (void *)table && release_list == list;
    report(ok, '1');

    reset();
    set_active(0, 1);
    set_active(2, 1);
    result = object_list_release_all(table, list);
    ok = result == 0
        && release_calls == 2
        && release_index[0] == 0 && release_index[1] == 2;
    report(ok, '2');

    reset();
    set_active(5, 1);
    result = object_list_release_all(table, list);
    ok = result == 0
        && release_calls == 1 && release_index[0] == 5
        && release_table == (void *)table && release_list == list;
    report(ok, '3');

    reset();
    set_active(0, 1);
    set_active(1, 1);
    release_result[0] = 0x0c;
    result = object_list_release_all(table, list);
    ok = result == 0x0c
        && release_calls == 1 && release_index[0] == 0;
    report(ok, '4');

    reset();
    set_active(0, 1);
    set_active(3, 1);
    set_active(4, 1);
    release_result[1] = 7;
    result = object_list_release_all(table, list);
    ok = result == 7
        && release_calls == 2
        && release_index[0] == 0 && release_index[1] == 3;
    report(ok, '5');

    reset();
    set_active(0x77, 1);
    result = object_list_release_all(table, list);
    ok = result == 0
        && release_calls == 1 && release_index[0] == 0x77;
    report(ok, '6');

    reset();
    set_active(0, 0x100);
    result = object_list_release_all(table, list);
    ok = result == 0
        && release_calls == 1 && release_index[0] == 0;
    report(ok, '7');

    put('D');
    quit();
    return 0;
}
