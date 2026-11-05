/* The test harness for the rebuilt `memory_error_report` module.
 *
 * The harness holds the record stub, the printer stub, and the exit stub.  The
 * printer stub records the stream, the format string, and the value after the
 * format of the first two calls.  The exit stub records its status and returns,
 * so one program can run every branch.
 *
 * The printer is a `__cdecl` function, so every argument is on the stack.  The
 * module declares the printer with a variable argument list.  The stub declares
 * the same first three arguments as a fixed list, so the value is readable even
 * for a call with only the format.  The program knows the value type of each
 * path, so the stub does not read the format.
 *
 * The program runs seven paths:
 *
 *     1  the allocate message with the handler name
 *     2  the no-slots message with the name, then the slot count
 *     3  the absent-handler message
 *     7  the damaged-control-block message
 *     9  the corrupted-structure message
 *     0  the default message, below the case range
 *     4  the default message, inside a case gap
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

typedef struct test_record {
    int reserved_00;
    int reserved_04;
    int slots;
    int status;
    char name[16];
} test_record;

void memory_error_report(test_record *record);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];

static int report_calls;
static char *stream_first;
static char *format_first;
static int value_first;
static char *stream_second;
static char *format_second;
static int value_second;
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
    if (report_calls == 0) {
        stream_first = stream;
        format_first = format;
        value_first = value;
    } else {
        stream_second = stream;
        format_second = format;
        value_second = value;
    }
    report_calls = report_calls + 1;
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
    stream_second = 0;
    format_second = 0;
    value_second = 0;
    exit_calls = 0;
    exit_status = 0;
    record->reserved_00 = 0;
    record->reserved_04 = 0;
    record->slots = 7;
    record->status = status;
    record->name[0] = 'N';
    record->name[1] = 'A';
    record->name[2] = 'M';
    record->name[3] = 'E';
    record->name[4] = 0;
}

static int one_message(char *format, int value)
{
    return report_calls == 1
        && stream_first == (char *)0x2fd2
        && same(format_first, format)
        && value_first == value
        && exit_calls == 1
        && exit_status == -1;
}

static int plain_message(char *format)
{
    return report_calls == 1
        && stream_first == (char *)0x2fd2
        && same(format_first, format)
        && exit_calls == 1
        && exit_status == -1;
}

int main(void)
{
    test_record record;

    prepare(&record, 1);
    memory_error_report(&record);
    put(one_message("Cannot allocate memory for: %s. Out of memory.\n",
                    (int)record.name) ? '1' : 'x');

    prepare(&record, 2);
    memory_error_report(&record);
    put(report_calls == 2
        && stream_first == (char *)0x2fd2
        && same(format_first,
                "No slots for allocating: %s. Resize memory handler.\n")
        && value_first == (int)record.name
        && stream_second == (char *)0x2fd2
        && same(format_second, "Actual number of slots: %d\n")
        && value_second == 7
        && exit_calls == 1
        && exit_status == -1 ? '2' : 'x');

    prepare(&record, 3);
    memory_error_report(&record);
    put(plain_message("Handler not present in memory management list.")
        ? '3' : 'x');

    prepare(&record, 7);
    memory_error_report(&record);
    put(plain_message("Memory control blocks damaged. System corrupted.")
        ? '7' : 'x');

    prepare(&record, 9);
    memory_error_report(&record);
    put(plain_message("Memory handler structures corrupted. Aborted")
        ? '9' : 'x');

    prepare(&record, 0);
    memory_error_report(&record);
    put(plain_message("Unknown error in memory management") ? '0' : 'x');

    prepare(&record, 4);
    memory_error_report(&record);
    put(plain_message("Unknown error in memory management") ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
