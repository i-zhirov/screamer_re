/* The test harness for the rebuilt `vis_tab_load` module.
 *
 * The harness holds the buffer pointer, the file stubs, and the three error
 * stubs.  The file stubs record their arguments.  The harness runs the module
 * with a successful open and then with a failed open:
 *
 *     0  the success path opened, read 0x27500 bytes into the buffer, and
 *        closed the handle
 *     1  the failure path ran the error report, the BIOS video call, and the
 *        exit with the value 0x16 (the exit stub returns, so the module then
 *        falls through to the read path, as the original does)
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void vis_tab_load(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_0007c1c4;

static char text[8];
static char vis_buffer[0x10];
static int open_result;
static char *got_path;
static char *got_mode;
static void *got_buffer;
static int got_size;
static int got_count;
static int got_stream;
static int got_close;
static int error_code;
static int video_code;
static int exit_code;
static int read_calls;
static int close_calls;
static int error_calls;
static int video_calls;
static int exit_calls;

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

int fread(void *buffer, int size, int count, int stream)
{
    got_buffer = buffer;
    got_size = size;
    got_count = count;
    got_stream = stream;
    read_calls = read_calls + 1;
    return count;
}

int fclose(int stream)
{
    got_close = stream;
    close_calls = close_calls + 1;
    return 0;
}

void error_report(int code)
{
    error_code = code;
    error_calls = error_calls + 1;
}

void bios_video(int code)
{
    video_code = code;
    video_calls = video_calls + 1;
}

void exit(int code)
{
    exit_code = code;
    exit_calls = exit_calls + 1;
}

static void reset(void)
{
    read_calls = 0;
    close_calls = 0;
    error_calls = 0;
    video_calls = 0;
    exit_calls = 0;
    got_buffer = 0;
    got_size = 0;
    got_count = 0;
    got_stream = 0;
    got_close = 0;
}

int main(void)
{
    DAT_0007c1c4 = vis_buffer;
    reset();
    open_result = 0x1234;
    vis_tab_load();
    put(same(got_path, "TABS\\VIS.TAB") && same(got_mode, "rb")
        && read_calls == 1 && got_buffer == vis_buffer && got_size == 1
        && got_count == 0x27500 && got_stream == 0x1234
        && close_calls == 1 && got_close == 0x1234
        && error_calls == 0 && exit_calls == 0 ? '0' : 'x');

    reset();
    open_result = 0;
    vis_tab_load();
    put(error_calls == 1 && error_code == 3
        && video_calls == 1 && video_code == 0x16
        && exit_calls == 1 && exit_code == 0x16 ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
