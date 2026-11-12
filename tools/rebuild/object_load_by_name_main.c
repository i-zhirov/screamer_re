/* The test harness for the rebuilt `object_load_by_name` module.
 *
 * The harness holds a small object table, the wanted name, the output slot, and
 * one stub for each callee.  The lookup stub records the call, the table, the
 * name, and the incoming slot value, then returns a scripted verdict and writes
 * a scripted source index.  The duplicate stub records the call, the table, the
 * two slot pointers, and their values.  The request stub records the call and
 * writes a scripted index into the output slot.  The load stub records the
 * call, the table, the start index, and the count.
 *
 * The harness runs eight paths:
 *
 *     0  the lookup finds the object: the duplicate result returns and the
 *        source index comes from the lookup
 *     1  the lookup finds the object and the duplicate returns 0
 *     2  the lookup misses: the request reserves an index and the load runs
 *        with that index and the count 1
 *     3  the lookup misses and the request returns 8: the load does not run
 *     4  the lookup misses and the request returns 3: the load does not run
 *     5  the lookup misses, the request succeeds, and the load returns 2
 *     6  the lookup misses, the request succeeds, and the load returns -1
 *     7  the lookup finds the source index 0 and the duplicate returns -1
 *     D  the exit marker
 *
 * Every path checks the result, the stub call counts, the recorded arguments,
 * and the slot values.
 */
#pragma off (check_stack)

int object_load_by_name(int *table, char *name, int *slot);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static int table[4];
static char object_name[8];
static int slot_value;

static int lookup_calls;
static int *lookup_table;
static char *lookup_name;
static int lookup_incoming;
static int script_lookup;
static int script_found;

static int duplicate_calls;
static int *duplicate_table;
static int *duplicate_output;
static int *duplicate_source;
static int duplicate_output_value;
static int duplicate_source_value;
static int script_duplicate;

static int request_calls;
static int *request_table;
static char *request_name;
static int *request_slot;
static int script_request;
static int script_request_out;

static int load_calls;
static int *load_table;
static int load_start;
static int load_count;
static int script_load;

int object_lookup(int *table, char *name, int *out)
{
    lookup_calls = lookup_calls + 1;
    lookup_table = table;
    lookup_name = name;
    lookup_incoming = *out;
    if (script_lookup != 0) {
        *out = script_found;
    }
    return script_lookup;
}

int object_duplicate(int *table, int *output, int *source)
{
    duplicate_calls = duplicate_calls + 1;
    duplicate_table = table;
    duplicate_output = output;
    duplicate_source = source;
    duplicate_output_value = *output;
    duplicate_source_value = *source;
    return script_duplicate;
}

int object_request(int *table, char *name, int *slot)
{
    request_calls = request_calls + 1;
    request_table = table;
    request_name = name;
    request_slot = slot;
    *slot = script_request_out;
    return script_request;
}

int object_load(int *table, int start, int count)
{
    load_calls = load_calls + 1;
    load_table = table;
    load_start = start;
    load_count = count;
    return script_load;
}

static char text[8];

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
    int index;

    for (index = 0; index < 4; index++) {
        table[index] = index + 1;
    }
    for (index = 0; index < 7; index++) {
        object_name[index] = "SHADOW"[index];
    }
    object_name[7] = 0;

    lookup_calls = 0;
    lookup_table = 0;
    lookup_name = 0;
    lookup_incoming = 0;
    script_lookup = 0;
    script_found = 0;

    duplicate_calls = 0;
    duplicate_table = 0;
    duplicate_output = 0;
    duplicate_source = 0;
    duplicate_output_value = 0;
    duplicate_source_value = 0;
    script_duplicate = 0;

    request_calls = 0;
    request_table = 0;
    request_name = 0;
    request_slot = 0;
    script_request = 0;
    script_request_out = 0;

    load_calls = 0;
    load_table = 0;
    load_start = 0;
    load_count = 0;
    script_load = 0;
}

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

int main(void)
{
    int result;
    int ok;

    reset();
    slot_value = 0x11;
    script_lookup = 1;
    script_found = 0x22;
    script_duplicate = 0x33;
    result = object_load_by_name(table, object_name, &slot_value);
    ok = result == 0x33 && lookup_calls == 1 && duplicate_calls == 1 &&
         request_calls == 0 && load_calls == 0 &&
         lookup_table == table && lookup_name == object_name &&
         lookup_incoming == 0x11 && duplicate_table == table &&
         duplicate_output == &slot_value && duplicate_output_value == 0x11 &&
         duplicate_source_value == 0x22;
    report(ok, '0');

    reset();
    slot_value = 0x21;
    script_lookup = 1;
    script_found = 0x31;
    script_duplicate = 0;
    result = object_load_by_name(table, object_name, &slot_value);
    ok = result == 0 && lookup_calls == 1 && duplicate_calls == 1 &&
         request_calls == 0 && load_calls == 0 &&
         lookup_incoming == 0x21 && duplicate_source_value == 0x31;
    report(ok, '1');

    reset();
    slot_value = 0x41;
    script_lookup = 0;
    script_request = 0;
    script_request_out = 0x55;
    script_load = 0;
    result = object_load_by_name(table, object_name, &slot_value);
    ok = result == 0 && lookup_calls == 1 && duplicate_calls == 0 &&
         request_calls == 1 && load_calls == 1 &&
         lookup_incoming == 0x41 && request_table == table &&
         request_name == object_name && request_slot == &slot_value &&
         slot_value == 0x55 && load_table == table &&
         load_start == 0x55 && load_count == 1;
    report(ok, '2');

    reset();
    slot_value = 0x61;
    script_lookup = 0;
    script_request = 8;
    script_request_out = 0x77;
    script_load = 0;
    result = object_load_by_name(table, object_name, &slot_value);
    ok = result == 8 && lookup_calls == 1 && duplicate_calls == 0 &&
         request_calls == 1 && load_calls == 0 &&
         slot_value == 0x77;
    report(ok, '3');

    reset();
    slot_value = 0x81;
    script_lookup = 0;
    script_request = 3;
    script_request_out = 0x99;
    script_load = 0;
    result = object_load_by_name(table, object_name, &slot_value);
    ok = result == 3 && lookup_calls == 1 && duplicate_calls == 0 &&
         request_calls == 1 && load_calls == 0 &&
         slot_value == 0x99;
    report(ok, '4');

    reset();
    slot_value = 0xa1;
    script_lookup = 0;
    script_request = 0;
    script_request_out = 0xb5;
    script_load = 2;
    result = object_load_by_name(table, object_name, &slot_value);
    ok = result == 2 && lookup_calls == 1 && duplicate_calls == 0 &&
         request_calls == 1 && load_calls == 1 &&
         load_start == 0xb5 && load_count == 1;
    report(ok, '5');

    reset();
    slot_value = 0xc1;
    script_lookup = 0;
    script_request = 0;
    script_request_out = 0xd5;
    script_load = -1;
    result = object_load_by_name(table, object_name, &slot_value);
    ok = result == -1 && lookup_calls == 1 && duplicate_calls == 0 &&
         request_calls == 1 && load_calls == 1 &&
         load_start == 0xd5 && load_count == 1;
    report(ok, '6');

    reset();
    slot_value = 0xe1;
    script_lookup = 1;
    script_found = 0;
    script_duplicate = -1;
    result = object_load_by_name(table, object_name, &slot_value);
    ok = result == -1 && lookup_calls == 1 && duplicate_calls == 1 &&
         request_calls == 0 && load_calls == 0 &&
         lookup_incoming == 0xe1 && duplicate_source_value == 0;
    report(ok, '7');

    put('D');
    quit();
    return 0;
}
