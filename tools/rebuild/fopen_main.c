/* The test harness for the rebuilt `fopen` module.
 *
 * The harness holds a stream record and the three stubs.  The stubs record
 * their arguments.  The harness calls the module along three paths:
 *
 *     0  the success path opened the path with the mode `rb`, stored the
 *        handle at `+0x00`, copied the category and the path, and searched the
 *        member
 *     1  the open-failure path stored the zero handle, copied the path to
 *        `+0x78`, set the state at `+0xac` to 1, and skipped the member search
 *     2  the member-failure path stored the handle, copied the two strings,
 *        searched the member, and returned -1
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int fopen(char *stream, char *path, char *name, char *category);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char record[0xc0];
static char *got_path;
static char *got_mode;
static char *got_stream;
static char *got_name;
static int handle_result;
static int member_result;
static int open_calls;
static int member_calls;

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

static void copy(char *destination, char *source)
{
    int index;

    for (index = 0; source[index] != 0; ++index) {
        destination[index] = source[index];
    }
    destination[index] = 0;
}

int file_open(char *path, char *mode)
{
    got_path = path;
    got_mode = mode;
    open_calls = open_calls + 1;
    return handle_result;
}

char *strcpy(char *destination, char *source)
{
    copy(destination, source);
    return destination;
}

int member_open(char *stream, char *name)
{
    got_stream = stream;
    got_name = name;
    member_calls = member_calls + 1;
    return member_result;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0xc0; ++index) {
        record[index] = 0;
    }
    for (index = 0; index < 0x34; ++index) {
        record[0x78 + index] = 0x5a;
        record[0x20 + index] = 0x5a;
        record[0x24 + index] = 0x5a;
    }
    *(int *)(record + 0xac) = 0x11223344;
    got_path = 0;
    got_mode = 0;
    got_stream = 0;
    got_name = 0;
    open_calls = 0;
    member_calls = 0;
}

int main(void)
{
    int result;

    reset();
    handle_result = 0x1234;
    member_result = 0;
    result = fopen((char *)record, "DATA\\MUSIC.DAT", "melodic.bnk",
                   "MELODIC_BANK");
    put(open_calls == 1 && same(got_path, "DATA\\MUSIC.DAT")
        && same(got_mode, "rb")
        && *(int *)record == 0x1234
        && same((char *)(record + 0x20), "MELODIC_BANK")
        && same((char *)(record + 0x24), "DATA\\MUSIC.DAT")
        && member_calls == 1 && got_stream == (char *)record
        && same(got_name, "melodic.bnk")
        && record[0x78] == 0x5a
        && *(int *)(record + 0xac) == 0x11223344
        && result == 0 ? '0' : 'x');

    reset();
    handle_result = 0;
    member_result = 0;
    result = fopen((char *)record, "DATA\\SOUND.DAT", "CRY.RAW", "SOUND_FX");
    put(open_calls == 1 && same(got_path, "DATA\\SOUND.DAT")
        && same(got_mode, "rb")
        && *(int *)record == 0
        && same((char *)(record + 0x78), "DATA\\SOUND.DAT")
        && *(int *)(record + 0xac) == 1
        && record[0x20] == 0x5a && record[0x24] == 0x5a
        && member_calls == 0
        && result == -1 ? '1' : 'x');

    reset();
    handle_result = 0x55;
    member_result = -1;
    result = fopen((char *)record, "DATA\\MUSIC.DAT", "drum.bnk", "DRUM_BANK");
    put(open_calls == 1 && *(int *)record == 0x55
        && same((char *)(record + 0x20), "DRUM_BANK")
        && same((char *)(record + 0x24), "DATA\\MUSIC.DAT")
        && member_calls == 1 && got_stream == (char *)record
        && same(got_name, "drum.bnk")
        && *(int *)(record + 0xac) == 0x11223344
        && result == -1 ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
