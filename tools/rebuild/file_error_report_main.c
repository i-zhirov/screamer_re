/* The test harness for the rebuilt `file_error_report` module.
 *
 * The harness holds the record stub, the printer stub, and the exit stub.  The
 * printer stub records the stream, the format string, and the value after the
 * format of the call.  The exit stub records its status and returns, so one
 * program can run every branch.
 *
 * The printer is a `__cdecl` function, so every argument is on the stack.  The
 * module declares the printer with a variable argument list.  The stub declares
 * the same first three arguments as a fixed list, so the value is readable even
 * for a call with only the format.  The program knows the value type of each
 * path, so the stub does not read the format.
 *
 * The program runs seven paths:
 *
 *     1  the main-file message with the interface name
 *     2  the absent-file message with the name
 *     3  the out-of-memory message
 *     4  the out-of-domain message
 *     5  the decode-memory message
 *     0  the default message, below the case range
 *     6  the default message, above the case range
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

typedef struct test_record {
    char reserved_00[0x78];
    char name[0x34];
    int status;
} test_record;

void file_error_report(test_record *record);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];

static int report_calls;
static char *stream_first;
static char *format_first;
static int value_first;
static int exit_calls;
static int exit_status;

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

void __cdecl report(char *stream, char *format, int value)
{
    report_calls = report_calls + 1;
    stream_first = stream;
    format_first = format;
    value_first = value;
}

void exit(int status)
{
    exit_calls = exit_calls + 1;
    exit_status = status;
}

static void prepare(test_record *record, int status)
{
    report_calls = 0;
    stream_first = 0;
    format_first = 0;
    value_first = 0;
    exit_calls = 0;
    exit_status = 0;
    record->status = status;
    record->name[0] = 'N';
    record->name[1] = 'A';
    record->name[2] = 'M';
    record->name[3] = 'E';
    record->name[4] = 0;
}

static int plain_message(char *format)
{
    return report_calls == 1
        && stream_first == (char *)0x2fd2
        && same(format_first, format)
        && exit_calls == 1
        && exit_status == -2;
}

int main(void)
{
    test_record record;
    int name;

    name = (int)record.name;

    prepare(&record, 1);
    file_error_report(&record);
    put(report_calls == 1
        && stream_first == (char *)0x2fd2
        && same(format_first, "Cannot open main file: %s")
        && value_first == name
        && exit_calls == 1
        && exit_status == -2 ? '1' : 'x');

    prepare(&record, 2);
    file_error_report(&record);
    put(report_calls == 1
        && stream_first == (char *)0x2fd2
        && same(format_first, "File %s not present in main file.")
        && value_first == name
        && exit_calls == 1
        && exit_status == -2 ? '2' : 'x');

    prepare(&record, 3);
    file_error_report(&record);
    put(plain_message("Out of memory.") ? '3' : 'x');

    prepare(&record, 4);
    file_error_report(&record);
    put(plain_message("Out of file domain.") ? '4' : 'x');

    prepare(&record, 5);
    file_error_report(&record);
    put(plain_message("Out of memory to decode file.") ? '5' : 'x');

    prepare(&record, 0);
    file_error_report(&record);
    put(report_calls == 1
        && stream_first == (char *)0x2fd2
        && same(format_first, "Unknown error in Engine interface <%s>.\n")
        && value_first == name
        && exit_calls == 1
        && exit_status == -2 ? '0' : 'x');

    prepare(&record, 6);
    file_error_report(&record);
    put(report_calls == 1
        && stream_first == (char *)0x2fd2
        && same(format_first, "Unknown error in Engine interface <%s>.\n")
        && value_first == name
        && exit_calls == 1
        && exit_status == -2 ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
