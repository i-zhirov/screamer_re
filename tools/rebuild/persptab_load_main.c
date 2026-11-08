/* The test harness for the rebuilt `persptab_load` module.
 *
 * The harness holds the resource structure, the `PERSPTAB` cell, the file
 * stubs, the resource stub, and the three error stubs.  The stubs record their
 * arguments.  The harness runs three paths:
 *
 *     0  the success path opened `TABS\persp01.tab` with the mode `rb`, read
 *        the resource of 0x40740 bytes, read 0x40740 bytes into the cell,
 *        advanced the cell by 0x40000, and closed the handle
 *     1  the failure path ran the BIOS video call with 3, the error report with
 *        0x16, and the exit with 0x16 (the exit stub returns, so the module then
 *        falls through to the read path, as the original does)
 *     2  the resource failure path ran `memory_error_report` with the resource
 *        structure and still ran the read, the advance, and the close
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void persptab_load(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_00021574;
char *DAT_000214ec;

static char text[8];
static char table[0x10];
static int open_result;
static void *read_result;
static char *got_path;
static char *got_mode;
static void *got_resource;
static char *got_name;
static unsigned got_size;
static void *got_buffer;
static int got_read_size;
static int got_count;
static int got_stream;
static int got_close;
static int read_calls;
static int close_calls;
static int bios_calls;
static int error_calls;
static int exit_calls;
static int memory_calls;
static int bios_code;
static int error_code;
static int exit_code;
static void *got_memory_resource;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int same(char *left, char *right)
{
    int index;

    for (index = 0; left[index] == right[index]; ++index) {
        if (left[index] == 0) {
            return 1;
        }
    }
    return 0;
}

int fopen(char *path, char *mode)
{
    got_path = path;
    got_mode = mode;
    return open_result;
}

void *resource_read(void *resource, char *name, unsigned size)
{
    got_resource = resource;
    got_name = name;
    got_size = size;
    read_calls = read_calls + 1;
    return read_result;
}

int fread(void *buffer, int size, int count, int stream)
{
    got_buffer = buffer;
    got_read_size = size;
    got_count = count;
    got_stream = stream;
    return count;
}

int fclose(int stream)
{
    got_close = stream;
    close_calls = close_calls + 1;
    return 0;
}

void memory_error_report(void *resource)
{
    got_memory_resource = resource;
    memory_calls = memory_calls + 1;
}

void bios_video(int code)
{
    bios_code = code;
    bios_calls = bios_calls + 1;
}

void error_report(int code)
{
    error_code = code;
    error_calls = error_calls + 1;
}

void exit(int code)
{
    exit_code = code;
    exit_calls = exit_calls + 1;
}

static void reset(void)
{
    DAT_00021574 = table;
    DAT_000214ec = 0;
    got_path = 0;
    got_mode = 0;
    got_resource = 0;
    got_name = 0;
    got_size = 0;
    got_buffer = 0;
    got_read_size = 0;
    got_count = 0;
    got_stream = 0;
    got_close = 0;
    read_calls = 0;
    close_calls = 0;
    bios_calls = 0;
    error_calls = 0;
    exit_calls = 0;
    memory_calls = 0;
    bios_code = 0;
    error_code = 0;
    exit_code = 0;
    got_memory_resource = 0;
}

int main(void)
{
    reset();
    open_result = 0x1234;
    read_result = table;
    persptab_load();
    put(same(got_path, "TABS\\persp01.tab") && same(got_mode, "rb")
        && read_calls == 1 && got_resource == (void *)table
        && got_name == (char *)0x1747 && got_size == 0x40740
        && got_buffer == table && got_read_size == 1 && got_count == 0x40740
        && got_stream == 0x1234 && DAT_000214ec == table + 0x40000
        && close_calls == 1 && got_close == 0x1234
        && bios_calls == 0 && error_calls == 0 && exit_calls == 0
        && memory_calls == 0 ? '0' : 'x');

    reset();
    open_result = 0;
    read_result = table;
    persptab_load();
    put(bios_calls == 1 && bios_code == 3
        && error_calls == 1 && error_code == 0x16
        && exit_calls == 1 && exit_code == 0x16 ? '1' : 'x');

    reset();
    open_result = 0x1234;
    read_result = 0;
    persptab_load();
    put(read_calls == 1 && memory_calls == 1
        && got_memory_resource == (void *)table
        && DAT_000214ec == (char *)0x40000 ? '2' : 'x');
    put('D');
    quit();
    return 0;
}
