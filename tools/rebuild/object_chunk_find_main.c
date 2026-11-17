/* The test harness for the rebuilt `object_chunk_find` module.
 *
 * The harness holds the 0x78-record object table at 0x1c bytes, two simulated
 * file records with their chunk offset tables, and the scratch buffer
 * `DAT_00007fe8`.  A record holds the file record at `+0x14`.  A file record
 * holds the chunk count at `+0x14` and the chunk offset table at `+0x18`.  A
 * chunk holds its name at `+0x68`.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero, so every
 * text lives in a data buffer that the harness fills at run time.
 *
 * The harness runs these paths:
 *
 *     0  the first chunk matches: the index 0 and the result 0
 *     1  the second chunk matches: the index 1
 *     2  the third chunk matches: the index 2
 *     3  no chunk matches: the result 7, the copied name, and the unchanged
 *        index
 *     4  the count is zero: the result 7 and the copied name
 *     5  the case differs: the compare ignores case and finds the index 0
 *     6  the position slot 5 selects its own file record
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

struct chunk_position {
    int slot;
    int index;
};

int object_chunk_find(char *name, struct chunk_position *position,
    struct object_record *objects);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define FILE_SIZE 0x400

char DAT_00007fe8[0x34];

static unsigned char table[0x78 * 0x1c];
static unsigned char file_zero[FILE_SIZE];
static unsigned char file_five[FILE_SIZE];
static char text[8];
static char name_buffer[0x20];
static char name_fauto[0x10];
static char name_mauto[0x10];
static char name_rauto[0x10];
static char name_missing[0x10];
static char name_none[0x10];
static char name_upper[0x10];
static char name_flx[0x10];

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

static void set_text(char *destination, char a, char b, char c, char d,
    char e, char f, char g, char h)
{
    destination[0] = a;
    destination[1] = b;
    destination[2] = c;
    destination[3] = d;
    destination[4] = e;
    destination[5] = f;
    destination[6] = g;
    destination[7] = h;
    destination[8] = 0;
}

static void init_text(void)
{
    set_text(name_fauto, 'f', 'a', 'u', 't', 'o', 0, 0, 0);
    set_text(name_mauto, 'M', 'V', '_', 'm', 'a', 'u', 't', 'o');
    set_text(name_rauto, 'r', 'a', 'u', 't', 'o', 0, 0, 0);
    set_text(name_missing, 'm', 'i', 's', 's', 'i', 'n', 'g', 0);
    set_text(name_none, 'n', 'o', 'n', 'e', 0, 0, 0, 0);
    set_text(name_upper, 'F', 'A', 'U', 'T', 'O', 0, 0, 0);
    set_text(name_flx, 'f', 'l', 'x', 0, 0, 0, 0, 0);
}

static void clear_bytes(unsigned char *data, int size)
{
    int index;

    for (index = 0; index < size; index++) {
        data[index] = 0;
    }
}

static void copy_text(char *destination, const char *source)
{
    while (*source != 0) {
        *destination++ = *source++;
    }
    *destination = 0;
}

static void set_count(unsigned char *file, int count)
{
    *(int *)(file + 0x14) = count;
}

static void set_offset(unsigned char *file, int index, int offset)
{
    *(int *)(file + 0x18 + index * 4) = offset;
}

static void set_chunk_name(unsigned char *file, int offset, char *name)
{
    copy_text((char *)(file + offset + 0x68), name);
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
    clear_bytes(file_zero, FILE_SIZE);
    clear_bytes(file_five, FILE_SIZE);
    for (index = 0; index < 0x34; index++) {
        DAT_00007fe8[index] = 0;
    }
}

static void build_main_file(void)
{
    record_at(0)->file = file_zero;
    set_count(file_zero, 3);
    set_offset(file_zero, 0, 0x100);
    set_offset(file_zero, 1, 0x180);
    set_offset(file_zero, 2, 0x200);
    set_chunk_name(file_zero, 0x100, name_fauto);
    set_chunk_name(file_zero, 0x180, name_mauto);
    set_chunk_name(file_zero, 0x200, name_rauto);
}

static int scratch_is(char *name)
{
    int index;

    for (index = 0;; index++) {
        if (DAT_00007fe8[index] != name[index]) {
            return 0;
        }
        if (name[index] == 0) {
            return 1;
        }
    }
}

int main(void)
{
    struct chunk_position position;
    struct object_record *objects;
    int result;
    int ok;

    objects = record_at(0);
    init_text();

    reset();
    build_main_file();
    copy_text(name_buffer, name_fauto);
    position.slot = 0;
    position.index = 0x55;
    result = object_chunk_find(name_buffer, &position, objects);
    ok = result == 0
        && position.index == 0
        && DAT_00007fe8[0] == 0;
    report(ok, '0');

    reset();
    build_main_file();
    copy_text(name_buffer, name_mauto);
    position.slot = 0;
    position.index = 0x55;
    result = object_chunk_find(name_buffer, &position, objects);
    ok = result == 0
        && position.index == 1;
    report(ok, '1');

    reset();
    build_main_file();
    copy_text(name_buffer, name_rauto);
    position.slot = 0;
    position.index = 0x55;
    result = object_chunk_find(name_buffer, &position, objects);
    ok = result == 0
        && position.index == 2;
    report(ok, '2');

    reset();
    build_main_file();
    copy_text(name_buffer, name_missing);
    position.slot = 0;
    position.index = 0x55;
    result = object_chunk_find(name_buffer, &position, objects);
    ok = result == 7
        && position.index == 0x55
        && scratch_is(name_missing);
    report(ok, '3');

    reset();
    record_at(0)->file = file_zero;
    set_count(file_zero, 0);
    copy_text(name_buffer, name_none);
    position.slot = 0;
    position.index = 0x55;
    result = object_chunk_find(name_buffer, &position, objects);
    ok = result == 7
        && position.index == 0x55
        && scratch_is(name_none);
    report(ok, '4');

    reset();
    build_main_file();
    file_zero[0x100] = 'Z';
    file_zero[0x101] = 'Z';
    file_zero[0x102] = 'Z';
    file_zero[0x103] = 0;
    copy_text(name_buffer, name_upper);
    position.slot = 0;
    position.index = 0x55;
    result = object_chunk_find(name_buffer, &position, objects);
    ok = result == 0
        && position.index == 0;
    report(ok, '5');

    reset();
    record_at(0)->file = file_zero;
    set_count(file_zero, 0);
    record_at(5)->file = file_five;
    set_count(file_five, 1);
    set_offset(file_five, 0, 0x120);
    set_chunk_name(file_five, 0x120, name_flx);
    copy_text(name_buffer, name_flx);
    position.slot = 5;
    position.index = 0x55;
    result = object_chunk_find(name_buffer, &position, objects);
    ok = result == 0
        && position.index == 0;
    report(ok, '6');

    put('D');
    quit();
    return 0;
}
