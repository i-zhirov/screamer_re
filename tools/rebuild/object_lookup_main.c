/* The test harness for the rebuilt `object_lookup` module.
 *
 * The harness holds the 0x78-record object table at 0x1c bytes.  A record
 * holds the name at `+0x00` and the file record at `+0x14`.  A zero file
 * record marks a free slot.
 *
 * The harness runs these paths:
 *
 *     0  the name matches the busy record 0: the result 1 and the slot 0
 *     1  the name matches the busy record 5: the result 1 and the slot 5
 *     2  the name matches the free record 2 only: the result 0 and the
 *        unchanged slot
 *     3  the name matches the busy record 3 with another case: the result 1
 *        and the slot 3
 *     4  no name matches: the result 0 and the unchanged slot
 *     5  the name matches the last busy record 0x77: the result 1 and the
 *        slot 0x77
 *     6  the name matches the free record 4 and the busy record 6: the result
 *        1 and the slot 6, because the scan skips the free record
 *     7  the empty name matches the busy record 7: the result 1 and the slot 7
 *     D  the exit marker
 */
#pragma off (check_stack)

struct object_record {
    char name[0x0c];
    int block;
    int chunks;
    int file;
    int reserved;
};

int object_lookup(char *table, char *name, int *slot);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static unsigned char table[0x78 * 0x1c];
static char text[8];
static char name_buffer[0x20];

int stricmp(const char *left, const char *right)
{
    unsigned char a;
    unsigned char b;

    for (;;) {
        a = (unsigned char)*left;
        b = (unsigned char)*right;
        if (a >= 'A' && a <= 'Z') {
            a = a + ('a' - 'A');
        }
        if (b >= 'A' && b <= 'Z') {
            b = b + ('a' - 'A');
        }
        if (a != b) {
            return (int)a - (int)b;
        }
        if (a == 0) {
            return 0;
        }
        left++;
        right++;
    }
}

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
}

static void set_name(char *buffer, char a, char b, char c, char d)
{
    buffer[0] = a;
    buffer[1] = b;
    buffer[2] = c;
    buffer[3] = d;
    buffer[4] = 0;
}

static void set_record_name(int index, char a, char b, char c, char d)
{
    set_name(record_at(index)->name, a, b, c, d);
}

static void set_busy(int index)
{
    record_at(index)->file = 0x12345678;
}

static void set_free(int index)
{
    record_at(index)->file = 0;
}

static int name_is(int index, char a, char b, char c, char d)
{
    struct object_record *record;

    record = record_at(index);
    return record->name[0] == a
        && record->name[1] == b
        && record->name[2] == c
        && record->name[3] == d
        && record->name[4] == 0;
}

int main(void)
{
    struct object_record *objects;
    int slot;
    int result;
    int ok;

    objects = record_at(0);

    reset();
    set_busy(0);
    set_record_name(0, 'f', 'a', 'u', 't');
    set_name(name_buffer, 'f', 'a', 'u', 't');
    slot = 0x55;
    result = object_lookup((char *)objects, name_buffer, &slot);
    ok = result == 1 && slot == 0 && name_is(0, 'f', 'a', 'u', 't');
    report(ok, '0');

    reset();
    set_busy(5);
    set_record_name(5, 'w', 'h', 'e', 'l');
    set_name(name_buffer, 'w', 'h', 'e', 'l');
    slot = 0x55;
    result = object_lookup((char *)objects, name_buffer, &slot);
    ok = result == 1 && slot == 5 && name_is(5, 'w', 'h', 'e', 'l');
    report(ok, '1');

    reset();
    set_free(2);
    set_record_name(2, 'a', 'l', 'p', 'h');
    set_name(name_buffer, 'a', 'l', 'p', 'h');
    slot = 0x55;
    result = object_lookup((char *)objects, name_buffer, &slot);
    ok = result == 0 && slot == 0x55 && record_at(2)->file == 0;
    report(ok, '2');

    reset();
    set_busy(3);
    set_record_name(3, 'M', 'i', 'X', 'e');
    set_name(name_buffer, 'm', 'i', 'x', 'E');
    slot = 0x55;
    result = object_lookup((char *)objects, name_buffer, &slot);
    ok = result == 1 && slot == 3 && name_is(3, 'M', 'i', 'X', 'e');
    report(ok, '3');

    reset();
    set_busy(0);
    set_record_name(0, 'o', 't', 'h', 'e');
    set_name(name_buffer, 'm', 'i', 's', 's');
    slot = 0x55;
    result = object_lookup((char *)objects, name_buffer, &slot);
    ok = result == 0 && slot == 0x55;
    report(ok, '4');

    reset();
    set_busy(0x77);
    set_record_name(0x77, 'l', 'a', 's', 't');
    set_name(name_buffer, 'l', 'a', 's', 't');
    slot = 0x55;
    result = object_lookup((char *)objects, name_buffer, &slot);
    ok = result == 1 && slot == 0x77 && name_is(0x77, 'l', 'a', 's', 't');
    report(ok, '5');

    reset();
    set_free(4);
    set_record_name(4, 'd', 'u', 'p', 'l');
    set_busy(6);
    set_record_name(6, 'd', 'u', 'p', 'l');
    set_name(name_buffer, 'd', 'u', 'p', 'l');
    slot = 0x55;
    result = object_lookup((char *)objects, name_buffer, &slot);
    ok = result == 1 && slot == 6 && name_is(6, 'd', 'u', 'p', 'l');
    report(ok, '6');

    reset();
    set_busy(7);
    set_name(name_buffer, 0, 0, 0, 0);
    slot = 0x55;
    result = object_lookup((char *)objects, name_buffer, &slot);
    ok = result == 1 && slot == 7 && record_at(7)->name[0] == 0;
    report(ok, '7');

    put('D');
    quit();
    return 0;
}
