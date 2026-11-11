/* The test harness for the rebuilt `exit_with_message` module.
 *
 * The harness supplies the console open, the message write, the finalizer, and
 * the process terminator as stubs.  The stubs record the device name, the
 * handle, the text, the length, the call order, and the exit code.  The
 * terminate stub returns, so one process can test every path.
 *
 * The paths:
 *
 *     0  a message with a trailing CR and LF and the code 3
 *     1  an empty message and the code 0
 *     2  a long run-time message and the code 1
 *     3  a one-byte message and the code -1
 *     D  the exit marker
 *
 * The stubs print no text, so the output holds only the path markers.
 */
#pragma off (check_stack)

void exit_with_message(const char *message, int status);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char open_name[8];
static char record[80];
static unsigned int record_length;
static int record_handle;
static int open_count;
static int write_count;
static int finish_count;
static int terminate_count;
static int terminate_status;

int exit_with_message_open(const char *name)
{
    int index;

    for (index = 0; index < 7 && name[index] != 0; index++) {
        open_name[index] = name[index];
    }
    open_name[index] = 0;
    open_count = open_count + 1;
    return 0x1234;
}

void exit_with_message_write(int handle, const char *message,
                             unsigned int length)
{
    unsigned int index;

    record_handle = handle;
    record_length = length;
    for (index = 0; index < length && index < 79; index++) {
        record[index] = message[index];
    }
    record[index] = 0;
    write_count = write_count + 1;
}

void exit_with_message_finish(void)
{
    finish_count = finish_count + 1;
}

void exit_with_message_terminate(int status)
{
    terminate_count = terminate_count + 1;
    terminate_status = status;
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

static void reset(void)
{
    int index;

    for (index = 0; index < 8; index++) {
        open_name[index] = 0;
    }
    for (index = 0; index < 80; index++) {
        record[index] = 0;
    }
    record_length = 0;
    record_handle = 0;
    open_count = 0;
    write_count = 0;
    finish_count = 0;
    terminate_count = 0;
    terminate_status = 0;
}

static int same(const char *left, const char *right)
{
    while (*left != 0 && *left == *right) {
        left = left + 1;
        right = right + 1;
    }
    return *left == *right;
}

static unsigned int length_of(const char *message)
{
    unsigned int length;

    length = 0;
    while (message[length] != 0) {
        length = length + 1;
    }
    return length;
}

static int check(const char *message, int status)
{
    return open_count == 1 && same(open_name, "con")
        && record_handle == 0x1234 && write_count == 1
        && record_length == length_of(message) && same(record, message)
        && finish_count == 1 && terminate_count == 1
        && terminate_status == status;
}

int main(void)
{
    reset();
    exit_with_message("abc\r\n", 3);
    put(check("abc\r\n", 3) ? '0' : 'x');

    reset();
    exit_with_message("", 0);
    put(check("", 0) ? '1' : 'x');

    reset();
    exit_with_message("Not enough memory to allocate file structures\r\n", 1);
    put(check("Not enough memory to allocate file structures\r\n", 1)
        ? '2' : 'x');

    reset();
    exit_with_message("x", -1);
    put(check("x", -1) ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
