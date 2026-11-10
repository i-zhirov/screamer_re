/* The test harness for the rebuilt `traffic_record_init` module.
 *
 * The harness holds the 0x2a8-byte traffic record, the 0x78-entry object table
 * at `DAT_00006530`, the model record table at `DAT_0000d25c`, one block object,
 * eight chunk records, and the two callee stubs.  The stubs record their three
 * arguments and the two loads return a scripted result.
 *
 * The harness runs six paths:
 *
 *     0  the normal path: the model 3, the two loads succeed, the block takes
 *        the model field, the four wheels take their chunk values, and the two
 *        centers and the span follow the rear wheel ranges
 *     1  the flags are negative: the function returns 0 and runs no call
 *     2  the model is 13: the function returns -1 and runs no call
 *     3  the first load returns 7: the function returns 7, runs one load, and
 *        writes no block or wheel field
 *     4  the second load returns 9: the function returns 9, runs two loads,
 *        and writes no block or wheel field
 *     5  the model 12 boundary passes: the model name and the block field use
 *        the last model record
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#define RECORD_SIZE 0x2a8
#define REC_FLAGS 0x00
#define REC_READY 0x18
#define REC_MODEL 0x1c
#define REC_MODEL_SLOT 0x64
#define REC_SHADOW_SLOT 0x6c
#define REC_WHEEL 0x8c
#define WHEEL_STRIDE 0x54
#define WHEEL_EDGE 0x00
#define WHEEL_RANGE 0x08
#define WHEEL_FIRST 0x2c
#define WHEEL_SECOND 0x34
#define REC_BOUND 0x1c4
#define BOUND_STRIDE 0x2c
#define BOUND_EDGE 0x00
#define BOUND_LIMIT 0x08
#define REC_ZERO_A 0x274
#define REC_CENTER_A 0x27c
#define REC_ZERO_B 0x280
#define REC_CENTER_B 0x288
#define REC_SPAN 0x2a4
#define MAX_CALLS 16

struct object_entry {
    int reserved_00[3];
    void *block;
    void *chunks;
    int reserved_14[2];
};

struct model_entry {
    char name[0xc];
    int animation;
    char rest[0x30];
};

struct chunk_entry {
    int reserved_00[5];
    int edge;
    int reserved_18;
    int range;
};

struct traffic_position {
    int slot;
    int chunk;
};

int traffic_record_init(void *record);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00006530[0x78 * 0x1c];
char DAT_0000d25c[16 * 0x40];

static char record[RECORD_SIZE];
static char text[8];
static int block_store[0x40];
static struct chunk_entry chunks[8];

static int load_calls;
static void *load_table[MAX_CALLS];
static char *load_name[MAX_CALLS];
static int *load_slot[MAX_CALLS];
static int load_result[MAX_CALLS];
static int load_slot_value[MAX_CALLS];

static int chunk_calls;
static void *chunk_table[MAX_CALLS];
static char *chunk_name[MAX_CALLS];
static struct traffic_position *chunk_position[MAX_CALLS];
static int chunk_value[MAX_CALLS];

int object_load_named(void *table, char *name, int *slot)
{
    int index;

    index = load_calls;
    if (index < MAX_CALLS) {
        load_table[index] = table;
        load_name[index] = name;
        load_slot[index] = slot;
    }
    load_calls = index + 1;
    if (index < MAX_CALLS) {
        if (load_result[index] == 0) {
            *slot = load_slot_value[index];
        }
        return load_result[index];
    }
    return 0;
}

int object_chunk_find(void *table, char *name,
                      struct traffic_position *position)
{
    int index;

    index = chunk_calls;
    if (index < MAX_CALLS) {
        chunk_table[index] = table;
        chunk_name[index] = name;
        chunk_position[index] = position;
        position->chunk = chunk_value[index];
    }
    chunk_calls = index + 1;
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

static int record_int(int offset)
{
    return *(int *)(record + offset);
}

static void record_set(int offset, int value)
{
    *(int *)(record + offset) = value;
}

static int absolute(int value)
{
    if (value < 0) {
        return -value;
    }
    return value;
}

static void reset(void)
{
    struct object_entry *table;
    struct model_entry *models;
    int index;

    table = (struct object_entry *)DAT_00006530;
    models = (struct model_entry *)DAT_0000d25c;
    for (index = 0; index < RECORD_SIZE; index++) {
        record[index] = 0;
    }
    for (index = 0; index < 0x78; index++) {
        table[index].block = 0;
        table[index].chunks = 0;
    }
    table[5].block = block_store;
    table[5].chunks = chunks;
    for (index = 0; index < 0x40; index++) {
        block_store[index] = 0;
    }
    for (index = 0; index < 8; index++) {
        chunks[index].edge = 100 + index;
        chunks[index].range = 200 + index;
    }
    for (index = 0; index < 16; index++) {
        models[index].animation = 0x1000 + index;
    }
    load_calls = 0;
    chunk_calls = 0;
    for (index = 0; index < MAX_CALLS; index++) {
        load_table[index] = 0;
        load_name[index] = 0;
        load_slot[index] = 0;
        load_result[index] = 0;
        load_slot_value[index] = 5;
        chunk_table[index] = 0;
        chunk_name[index] = 0;
        chunk_position[index] = 0;
        chunk_value[index] = index;
    }
}

static int loads_ok(int model)
{
    int index;
    int ok;

    ok = load_calls == 2
        && load_table[0] == (void *)0x6530
        && load_name[0] == (char *)(0xd25c + (model << 6))
        && load_slot[0] == (int *)(record + REC_MODEL_SLOT)
        && load_table[1] == (void *)0x6530
        && load_name[1] == (char *)0x5fc
        && load_slot[1] == (int *)(record + REC_SHADOW_SLOT)
        && record_int(REC_MODEL_SLOT) == 5
        && record_int(REC_SHADOW_SLOT) == 6;
    for (index = 0; index < 4; index++) {
        if (chunk_table[index * 2] != (void *)0x6530) {
            ok = 0;
        }
        if (chunk_table[index * 2 + 1] != (void *)0x6530) {
            ok = 0;
        }
        if (chunk_name[index * 2] != (char *)(0x1d5c + index * 8)) {
            ok = 0;
        }
        if (chunk_name[index * 2 + 1] != (char *)(0x1d60 + index * 8)) {
            ok = 0;
        }
        if (chunk_position[index * 2]
            != (struct traffic_position *)(record + REC_WHEEL
                                           + index * WHEEL_STRIDE
                                           + WHEEL_FIRST)) {
            ok = 0;
        }
        if (chunk_position[index * 2 + 1]
            != (struct traffic_position *)(record + REC_WHEEL
                                           + index * WHEEL_STRIDE
                                           + WHEEL_SECOND)) {
            ok = 0;
        }
    }
    return ok;
}

static int wheels_ok(void)
{
    int index;
    int offset;
    int ok;
    int wheel;

    ok = chunk_calls == 8;
    for (index = 0; index < 4; index++) {
        wheel = REC_WHEEL + index * WHEEL_STRIDE;
        if (record_int(wheel + WHEEL_EDGE) != 100 + index * 2) {
            ok = 0;
        }
        if (record_int(wheel + WHEEL_RANGE) != 200 + index * 2) {
            ok = 0;
        }
        if (record_int(wheel + WHEEL_FIRST) != 5) {
            ok = 0;
        }
        if (record_int(wheel + WHEEL_SECOND) != 5) {
            ok = 0;
        }
        if (record_int(wheel + WHEEL_FIRST + 4) != index * 2) {
            ok = 0;
        }
        if (record_int(wheel + WHEEL_SECOND + 4) != index * 2 + 1) {
            ok = 0;
        }
        offset = (index & 1) ? 150 : -250;
        if (record_int(REC_BOUND + index * BOUND_STRIDE + BOUND_EDGE)
            != 100 + index * 2) {
            ok = 0;
        }
        if (record_int(REC_BOUND + index * BOUND_STRIDE + BOUND_LIMIT)
            != 200 + index * 2 + offset) {
            ok = 0;
        }
    }
    return ok;
}

static int tail_ok(void)
{
    int bound_two;
    int bound_three;
    int center_a;
    int center_b;
    int span;

    bound_two = record_int(REC_BOUND + 2 * BOUND_STRIDE + BOUND_LIMIT);
    bound_three = record_int(REC_BOUND + 3 * BOUND_STRIDE + BOUND_LIMIT);
    center_a = bound_two + (bound_three - bound_two) / 4;
    center_b = bound_three + (bound_two - bound_three) / 4;
    span = absolute((center_a - center_b) / 2);
    return record_int(REC_ZERO_A) == 0
        && record_int(REC_ZERO_B) == 0
        && record_int(REC_CENTER_A) == center_a
        && record_int(REC_CENTER_B) == center_b
        && record_int(REC_SPAN) == span
        && record_int(REC_READY) == 0;
}

static int normal_ok(int model)
{
    return loads_ok(model)
        && wheels_ok()
        && block_store[8] == 0x1000 + model
        && tail_ok();
}

int main(void)
{
    int result;
    int ok;

    reset();
    record_set(REC_MODEL, 3);
    load_slot_value[1] = 6;
    result = traffic_record_init(record);
    put(result == 0 && normal_ok(3) ? '0' : 'x');

    reset();
    record_set(REC_FLAGS, -1);
    record_set(REC_MODEL, 3);
    record_set(REC_READY, 0x55);
    result = traffic_record_init(record);
    put(result == 0 && load_calls == 0 && chunk_calls == 0
        && record_int(REC_READY) == 0x55 ? '1' : 'x');

    reset();
    record_set(REC_MODEL, 13);
    record_set(REC_READY, 0x55);
    result = traffic_record_init(record);
    put(result == -1 && load_calls == 0 && chunk_calls == 0
        && record_int(REC_READY) == 0x55 ? '2' : 'x');

    reset();
    record_set(REC_MODEL, 3);
    record_set(REC_READY, 0x55);
    load_result[0] = 7;
    result = traffic_record_init(record);
    put(result == 7 && load_calls == 1 && chunk_calls == 0
        && block_store[8] == 0 && record_int(REC_MODEL_SLOT) == 0
        && record_int(REC_READY) == 0x55 ? '3' : 'x');

    reset();
    record_set(REC_MODEL, 3);
    record_set(REC_READY, 0x55);
    load_result[1] = 9;
    result = traffic_record_init(record);
    put(result == 9 && load_calls == 2 && chunk_calls == 0
        && block_store[8] == 0 && record_int(REC_SHADOW_SLOT) == 0
        && record_int(REC_READY) == 0x55 ? '4' : 'x');

    reset();
    record_set(REC_MODEL, 12);
    load_slot_value[1] = 6;
    result = traffic_record_init(record);
    ok = result == 0 && normal_ok(12) && block_store[8] == 0x1000 + 12;
    put(ok ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
