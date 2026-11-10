/* The test harness for the rebuilt `object_request` module.
 *
 * The harness holds the 0x78-record object table at 0x1c bytes and the scratch
 * buffer `DAT_00007fe8`.  A record holds the name at `+0x00` and the file
 * record at `+0x14`.
 *
 * The harness runs these paths:
 *
 *     0  slot 0 is free: the name and the value 1 go into slot 0 and the call
 *        returns 0
 *     1  slot 0 is busy: the scan selects slot 1 and returns 0
 *     2  every slot is busy: the call returns 8 and copies the name to the
 *        scratch buffer
 *     3  a one-letter name stops the copy at the null and leaves the byte after
 *        the null unchanged
 *     4  slots 0 and 1 are busy: the scan selects slot 2
 *     5  slots 0 through 0x76 are busy: the scan selects the last slot 0x77
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

int object_request(struct object_record *objects, char *name, int *slot);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00007fe8[0x34];

static unsigned char table[0x78 * 0x1c];
static char text[8];
static char name_buffer[0x20];

static struct object_record *record_at(int index)
{
    return (struct object_record *)(table + index * 0x1c);
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
    for (index = 0; index < 0x34; index++) {
        DAT_00007fe8[index] = 0;
    }
}

static void set_busy(int index)
{
    record_at(index)->file = (void *)0x12345678;
}

static void set_name(char *buffer, char a, char b, char c)
{
    buffer[0] = a;
    buffer[1] = b;
    buffer[2] = c;
    buffer[3] = 0;
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

static int scratch_is(char a, char b, char c)
{
    return DAT_00007fe8[0] == a
        && DAT_00007fe8[1] == b
        && DAT_00007fe8[2] == c
        && DAT_00007fe8[3] == 0;
}

int main(void)
{
    struct object_record *objects;
    int slot;
    int result;
    int ok;
    int index;

    objects = record_at(0);

    reset();
    set_name(name_buffer, 'A', 'B', 'C');
    slot = 0x55;
    result = object_request(objects, name_buffer, &slot);
    ok = result == 0
        && slot == 0
        && name_is(0, 'A', 'B', 'C')
        && record_at(0)->file == (void *)1
        && record_at(1)->name[0] == 0
        && DAT_00007fe8[0] == 0;
    report(ok, '0');

    reset();
    set_busy(0);
    set_busy(2);
    set_name(name_buffer, 'X', 'Y', 'Z');
    slot = 0x55;
    result = object_request(objects, name_buffer, &slot);
    ok = result == 0
        && slot == 1
        && name_is(1, 'X', 'Y', 'Z')
        && record_at(1)->file == (void *)1
        && record_at(0)->name[0] == 0;
    report(ok, '1');

    reset();
    for (index = 0; index < 0x78; index++) {
        set_busy(index);
    }
    set_name(name_buffer, 'F', 'U', 'L');
    slot = 0;
    result = object_request(objects, name_buffer, &slot);
    ok = result == 8
        && slot == 0x78
        && scratch_is('F', 'U', 'L')
        && record_at(0)->name[0] == 0;
    report(ok, '2');

    reset();
    record_at(0)->name[2] = 'Z';
    name_buffer[0] = 'Q';
    name_buffer[1] = 0;
    slot = 0x55;
    result = object_request(objects, name_buffer, &slot);
    ok = result == 0
        && slot == 0
        && record_at(0)->name[0] == 'Q'
        && record_at(0)->name[1] == 0
        && record_at(0)->name[2] == 'Z'
        && record_at(0)->file == (void *)1;
    report(ok, '3');

    reset();
    set_busy(0);
    set_busy(1);
    set_name(name_buffer, 'M', 'N', 'O');
    slot = 0;
    result = object_request(objects, name_buffer, &slot);
    ok = result == 0
        && slot == 2
        && name_is(2, 'M', 'N', 'O')
        && record_at(2)->file == (void *)1;
    report(ok, '4');

    reset();
    for (index = 0; index < 0x77; index++) {
        set_busy(index);
    }
    set_name(name_buffer, 'L', 'S', 'T');
    slot = 0;
    result = object_request(objects, name_buffer, &slot);
    ok = result == 0
        && slot == 0x77
        && name_is(0x77, 'L', 'S', 'T')
        && record_at(0x77)->file == (void *)1;
    report(ok, '5');

    put('D');
    quit();
    return 0;
}
