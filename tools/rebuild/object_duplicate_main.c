/* The test harness for the rebuilt `object_duplicate` module.
 *
 * The harness holds the 0x78-record object table, one source block, one file
 * record, and two callee stubs.  Each stub records its arguments and returns a
 * scripted value.
 *
 * The harness runs six paths:
 *
 *     0  the free slot 0 takes the source at slot 1, the resource size comes
 *        from the source count 5, and the new record takes the block, the name,
 *        the chunk table, and the shared file record
 *     1  slot 0 is busy, so the scan selects slot 1; the source at slot 2 gives
 *        the size, the block index, and the name
 *     2  the resource returns zero, so the function returns 6, sets the block
 *        to zero, and copies no name
 *     3  every slot is busy, so the function returns 8 and keeps the count
 *        0x78 in the output index
 *     4  the source count 0 gives the minimum size 0x60 and the block index 0
 *     5  the scan selects slot 2, and the name copy stops at the source null
 *        and leaves the byte after the null
 *     D  the exit marker
 */
#pragma off (check_stack)

struct object_record {
    char name[0x0c];
    void *block;
    void *chunks;
    void *file;
    int reserved;
};

int object_duplicate(struct object_record *objects, int *out_index,
                     int *source_index);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_0000801c;

static unsigned char table[0x78 * 0x1c];
static unsigned char source_block[0x10];
static unsigned char file_record[0x20];
static unsigned char block_buffer[0x400];

static int resource_calls;
static void *resource_handle;
static char *resource_name;
static int resource_size;
static void *resource_result;

static int init_calls;
static void *init_object;
static int init_index;

static char text[8];

static struct object_record *record_at(int index)
{
    return (struct object_record *)(table + index * 0x1c);
}

void *resource_get(void *resource, char *name, int size)
{
    resource_calls++;
    resource_handle = resource;
    resource_name = name;
    resource_size = size;
    return resource_result;
}

void object_init_from_file(void *object, int index)
{
    init_calls++;
    init_object = object;
    init_index = index;
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

    for (index = 0; index < 0x78 * 0x1c; index++) {
        table[index] = 0;
    }
    for (index = 0; index < 0x10; index++) {
        source_block[index] = 0;
    }
    for (index = 0; index < 0x20; index++) {
        file_record[index] = 0;
    }
    for (index = 0; index < 0x400; index++) {
        block_buffer[index] = 0;
    }

    DAT_0000801c = (void *)0x22222222;

    resource_calls = 0;
    resource_handle = 0;
    resource_name = 0;
    resource_size = 0;
    resource_result = (void *)block_buffer;

    init_calls = 0;
    init_object = 0;
    init_index = 0;
}

static void set_file_count(int count)
{
    *(int *)(file_record + 0x14) = count;
}

static void set_source(int index, int block_index, char a, char b, char c)
{
    struct object_record *record;

    record = record_at(index);
    record->file = file_record;
    record->block = source_block;
    *(int *)source_block = block_index;
    record->name[0] = a;
    record->name[1] = b;
    record->name[2] = c;
    record->name[3] = 0;
}

static int name_is(int index, char a, char b, char c)
{
    struct object_record *record;

    record = record_at(index);
    return record->name[0] == a
        && record->name[1] == b
        && record->name[2] == c
        && record->name[3] == 0;
}

int main(void)
{
    struct object_record *objects;
    int out;
    int source;
    int result;
    int ok;
    int index;

    objects = record_at(0);

    reset();
    set_file_count(5);
    set_source(1, 7, 'S', 'R', 'C');
    out = 0x55;
    source = 1;
    result = object_duplicate(objects, &out, &source);
    ok = result == 0
        && out == 0
        && resource_calls == 1
        && resource_handle == DAT_0000801c
        && resource_name == (char *)0x3ca
        && resource_size == 0x100
        && init_calls == 1
        && init_object == (void *)block_buffer
        && init_index == 7
        && record_at(0)->block == (void *)block_buffer
        && record_at(0)->chunks == (void *)(block_buffer + 0x40)
        && record_at(0)->file == (void *)file_record
        && name_is(0, 'S', 'R', 'C')
        && record_at(1)->file == (void *)file_record
        && name_is(1, 'S', 'R', 'C');
    report(ok, '0');

    reset();
    set_file_count(3);
    set_source(2, 4, 'X', 'Y', 'Z');
    record_at(0)->file = file_record;
    out = 0x99;
    source = 2;
    result = object_duplicate(objects, &out, &source);
    ok = result == 0
        && out == 1
        && resource_calls == 1
        && resource_size == 0xc0
        && init_calls == 1
        && init_index == 4
        && record_at(1)->block == (void *)block_buffer
        && record_at(1)->chunks == (void *)(block_buffer + 0x40)
        && record_at(1)->file == (void *)file_record
        && name_is(1, 'X', 'Y', 'Z');
    report(ok, '1');

    reset();
    set_file_count(5);
    set_source(1, 7, 'S', 'R', 'C');
    record_at(0)->name[0] = 'O';
    record_at(0)->name[1] = 'L';
    record_at(0)->name[2] = 'D';
    record_at(0)->name[3] = 0;
    resource_result = 0;
    out = 0x11;
    source = 1;
    result = object_duplicate(objects, &out, &source);
    ok = result == 6
        && out == 0
        && resource_calls == 1
        && init_calls == 0
        && record_at(0)->block == 0
        && record_at(0)->file == 0
        && name_is(0, 'O', 'L', 'D');
    report(ok, '2');

    reset();
    set_file_count(1);
    for (index = 0; index < 0x78; index++) {
        record_at(index)->file = file_record;
    }
    out = 0;
    source = 0;
    result = object_duplicate(objects, &out, &source);
    ok = result == 8
        && out == 0x78
        && resource_calls == 0
        && init_calls == 0;
    report(ok, '3');

    reset();
    set_file_count(0);
    set_source(1, 0, 'A', 'B', 'C');
    out = 0;
    source = 1;
    result = object_duplicate(objects, &out, &source);
    ok = result == 0
        && out == 0
        && resource_calls == 1
        && resource_size == 0x60
        && init_calls == 1
        && init_index == 0
        && record_at(0)->block == (void *)block_buffer
        && name_is(0, 'A', 'B', 'C');
    report(ok, '4');

    reset();
    set_file_count(2);
    set_source(3, 9, 'A', 'B', 0);
    record_at(0)->file = file_record;
    record_at(1)->file = file_record;
    record_at(2)->name[0] = 'Z';
    record_at(2)->name[1] = 'Z';
    record_at(2)->name[2] = 'Z';
    record_at(2)->name[3] = 'Z';
    out = 0;
    source = 3;
    result = object_duplicate(objects, &out, &source);
    ok = result == 0
        && out == 2
        && resource_calls == 1
        && resource_size == 0xa0
        && init_calls == 1
        && init_index == 9
        && record_at(2)->block == (void *)block_buffer
        && record_at(2)->name[0] == 'A'
        && record_at(2)->name[1] == 'B'
        && record_at(2)->name[2] == 0
        && record_at(2)->name[3] == 'Z'
        && record_at(2)->file == (void *)file_record;
    report(ok, '5');

    put('D');
    quit();
    return 0;
}
