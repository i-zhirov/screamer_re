/* The test harness for the rebuilt `object_init_from_file` module.
 *
 * The harness holds the file record, the 0x100-byte object record, and the
 * 1002-slot handle table.  The message stub records its call count.  The
 * harness runs four paths:
 *
 *     0  the index 3 selects a file with two chunks.  The object fields are
 *        set, the two chunk records copy the file, the count, and the six
 *        header words, and the end marker follows the second record
 *     1  a file with zero chunks writes the end marker at object + 0x44
 *     2  the index 1001 is above the bound, so the message call runs and the
 *        function continues with the handle at that index
 *     3  the index 1000 is the bound, so the message call does not run
 *     D  the exit marker
 *
 * The object record starts filled with 0x5a, so a field that the function
 * does not write fails the check.  The markers go through the data buffer
 * `text`, because the `print` helper of this harness writes garbage for a
 * string literal (the same fault shows in the output of the driver at
 * work/rebuild/out.txt).
 */
#pragma off (check_stack)

void object_init_from_file(void *object, int index);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int *DAT_00021540;

static char text[8];
static unsigned char file_data[0x200];
static unsigned char object[0x100];
static int handles[1002];
static int report_calls;
static char *report_text;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void fill(unsigned char *buffer, int size, int value)
{
    int index;

    for (index = 0; index < size; ++index) {
        buffer[index] = (unsigned char)value;
    }
}

static void set_word(unsigned char *buffer, int offset, int value)
{
    *(int *)(buffer + offset) = value;
}

static int get_word(unsigned char *buffer, int offset)
{
    return *(int *)(buffer + offset);
}

static void set_header(int offset, int base)
{
    set_word(file_data, offset + 0x04, base + 1);
    set_word(file_data, offset + 0x08, base + 2);
    set_word(file_data, offset + 0x0c, base + 3);
    set_word(file_data, offset + 0x10, base + 4);
    set_word(file_data, offset + 0x14, base + 5);
    set_word(file_data, offset + 0x18, base + 6);
}

static void reset(int count)
{
    fill(file_data, 0x200, 0);
    fill(object, 0x100, 0x5a);
    fill((unsigned char *)handles, 1002 * 4, 0);
    set_word(file_data, 0x14, count);
    report_calls = 0;
    report_text = 0;
    DAT_00021540 = handles;
}

static int object_fields(int index)
{
    return get_word(object, 0x00) == index
        && get_word(object, 0x04) == 1
        && get_word(object, 0x08) == 0
        && get_word(object, 0x0c) == 0
        && get_word(object, 0x10) == 0
        && get_word(object, 0x14) == 0
        && get_word(object, 0x18) == 0
        && get_word(object, 0x1c) == 0
        && get_word(object, 0x20) == -1
        && get_word(object, 0x24) == 0
        && get_word(object, 0x28) == 0
        && get_word(object, 0x2c) == 0
        && get_word(object, 0x30) == 0
        && get_word(object, 0x34) == 0
        && get_word(object, 0x38) == 0;
}

static int entry_is(int slot, int count, int header_offset)
{
    unsigned char *entry;

    entry = object + 0x40 + slot * 0x20;
    return get_word(entry, 0x00) == (int)file_data
        && get_word(entry, 0x04) == count
        && get_word(entry, 0x08) == get_word(file_data, header_offset + 0x04)
        && get_word(entry, 0x0c) == get_word(file_data, header_offset + 0x08)
        && get_word(entry, 0x10) == get_word(file_data, header_offset + 0x0c)
        && get_word(entry, 0x14) == get_word(file_data, header_offset + 0x10)
        && get_word(entry, 0x18) == get_word(file_data, header_offset + 0x14)
        && get_word(entry, 0x1c) == get_word(file_data, header_offset + 0x18);
}

int report_message(char *message)
{
    report_calls = report_calls + 1;
    report_text = message;
    return 0;
}

int main(void)
{
    reset(2);
    handles[3] = (int)file_data;
    set_word(file_data, 0x18, 0x40);
    set_word(file_data, 0x1c, 0x80);
    set_word(file_data, 0x40 + 0x0c, 0x100);
    set_word(file_data, 0x80 + 0x0c, 0x120);
    set_header(0x100, 0x10);
    set_header(0x120, 0x20);
    object_init_from_file(object, 3);
    put(object_fields(3)
        && entry_is(0, 2, 0x100) && entry_is(1, 2, 0x120)
        && get_word(object, 0x40 + 2 * 0x20 + 0x04) == -1
        && report_calls == 0 ? '0' : 'x');

    reset(0);
    handles[5] = (int)file_data;
    object_init_from_file(object, 5);
    put(object_fields(5)
        && get_word(object, 0x40 + 0x04) == -1
        && report_calls == 0 ? '1' : 'x');

    reset(1);
    handles[1001] = (int)file_data;
    set_word(file_data, 0x18, 0x40);
    set_word(file_data, 0x40 + 0x0c, 0x100);
    set_header(0x100, 0x30);
    object_init_from_file(object, 1001);
    put(object_fields(1001) && entry_is(0, 1, 0x100)
        && get_word(object, 0x40 + 0x20 + 0x04) == -1
        && report_calls == 1 && report_text != 0 ? '2' : 'x');

    reset(0);
    handles[1000] = (int)file_data;
    object_init_from_file(object, 1000);
    put(object_fields(1000)
        && get_word(object, 0x40 + 0x04) == -1
        && report_calls == 0 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
