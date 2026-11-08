/* The test harness for the rebuilt `__fprtf` module.
 *
 * The harness holds a file record and the three stubs.  The stubs record their
 * arguments.  The format stub can set the 0x20 flag to test the error return.
 * The harness runs the normal path, the null-buffer path, the 0xd flag path,
 * and the error path:
 *
 *     0  the normal path clears and restores the 0x30 flags and returns the
 *        format result
 *     1  a null buffer calls the allocate stub
 *     2  the 0xd bit 2 clears to bit 0, then restores and calls the restore stub
 *     3  the 0x20 flag after the format returns -1
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int __fprtf(void *file, char *format, void *args);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_001c428e[4];

static char text[8];
static unsigned char file[0x20];
static char format[4];
static char arguments[4];
static void *got_file;
static char *got_format;
static void *got_args;
static void *got_table;
static int format_result;
static int error_flag;
static int format_calls;
static int alloc_calls;
static int restore_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void stream_alloc(void *target)
{
    got_file = target;
    alloc_calls = alloc_calls + 1;
}

int __doformat(void *target, char *fmt, void *args, void *table)
{
    got_file = target;
    got_format = fmt;
    got_args = args;
    got_table = table;
    format_calls = format_calls + 1;
    if (error_flag != 0) {
        *(unsigned char *)((char *)target + 0xc) |= 0x20;
    }
    return format_result;
}

void stream_restore(void *target)
{
    got_file = target;
    restore_calls = restore_calls + 1;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x20; ++index) {
        file[index] = 0;
    }
    *(void **)(file + 8) = file;
    *(unsigned int *)(file + 0xc) = 0x30;
    file[0xd] = 0;
    format_calls = 0;
    alloc_calls = 0;
    restore_calls = 0;
    got_file = 0;
    got_format = 0;
    got_args = 0;
    got_table = 0;
    format_result = 0x1234;
    error_flag = 0;
}

int main(void)
{
    int result;

    reset();
    result = __fprtf(file, format, arguments);
    put(result == 0x1234 && format_calls == 1 && alloc_calls == 0
        && restore_calls == 0 && got_file == file && got_format == format
        && got_args == arguments && got_table == DAT_001c428e
        && *(unsigned int *)(file + 0xc) == 0x30 ? '0' : 'x');

    reset();
    *(void **)(file + 8) = 0;
    result = __fprtf(file, format, arguments);
    put(result == 0x1234 && alloc_calls == 1 ? '1' : 'x');

    reset();
    file[0xd] = 4;
    result = __fprtf(file, format, arguments);
    put(result == 0x1234 && format_calls == 1 && restore_calls == 1
        && (file[0xd] & 4) != 0 ? '2' : 'x');

    reset();
    error_flag = 1;
    result = __fprtf(file, format, arguments);
    put(result == -1 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
