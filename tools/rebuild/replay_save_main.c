/* The test harness for the rebuilt `replay_save` module.
 *
 * The harness holds the base path, the six setup words, the five block
 * buffers, and the frame buffer pointer.  The stubs record the `sprintf`,
 * `fopen`, `fwrite`, and `fclose` arguments.
 *
 * The harness runs three paths:
 *
 *     0  the open fails: the function returns 1 and writes nothing
 *     1  the open succeeds: the header copy and five block writes run, and the
 *        fifth write takes its count from DAT_00018e70
 *     2  the frame count is zero: the fifth write still runs with count 0
 *     D  the exit marker
 *
 * The harness sets the base path text in the data buffer DAT_00007edc.
 */
#pragma off (check_stack)

#include <stdarg.h>

int replay_save(char *name);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define BLOCK1_SIZE 0xa8
#define BLOCK2_SIZE 0x460
#define BLOCK3_SIZE 0x28
#define HEADER_SIZE 0x18
#define FRAME_SIZE 0x14
#define MAX_FRAMES 8

unsigned char DAT_00007edc[0x40];
int DAT_00018e5c;
int DAT_00018e70;
int DAT_00018e74[6];
int DAT_0001d2a0;
int DAT_0001d2b0;
int DAT_0001d2b4;
int DAT_0001d260;
int DAT_0001d274;
unsigned char DAT_000197d8[BLOCK1_SIZE];
unsigned char DAT_000192a4[BLOCK2_SIZE];
unsigned char DAT_00019704[BLOCK3_SIZE];

static unsigned char replay_frames[MAX_FRAMES][FRAME_SIZE];

static char text[8];

static int format_calls;
static char *format_text;
static void *format_first;
static void *format_second;
static int open_calls;
static char *open_path;
static char *open_mode;
static int open_result;
static int write_calls;
static const void *write_ptr[8];
static unsigned int write_size[8];
static unsigned int write_count[8];
static int close_calls;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int text_equal(const char *left, const char *right)
{
    while (*left != 0 && *left == *right) {
        left++;
        right++;
    }
    return *left == *right;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;

    format_calls++;
    format_text = (char *)format;
    va_start(arguments, format);
    format_first = va_arg(arguments, void *);
    format_second = va_arg(arguments, void *);
    va_end(arguments);
    buffer[0] = 'P';
    buffer[1] = 'A';
    buffer[2] = 'T';
    buffer[3] = 'H';
    buffer[4] = 0;
    return 4;
}

void *fopen(const char *path, const char *mode)
{
    open_calls++;
    open_path = (char *)path;
    open_mode = (char *)mode;
    return open_result ? (void *)1 : 0;
}

unsigned int fwrite(const void *ptr, unsigned int size, unsigned int count,
    void *stream)
{
    if (write_calls < 8) {
        write_ptr[write_calls] = ptr;
        write_size[write_calls] = size;
        write_count[write_calls] = count;
    }
    write_calls++;
    return count;
}

int fclose(void *stream)
{
    close_calls++;
    return 0;
}

static void reset(void)
{
    int index;

    for (index = 0; index < (int)sizeof(replay_frames); index++) {
        ((unsigned char *)replay_frames)[index] = 0;
    }
    for (index = 0; index < 6; index++) {
        DAT_00018e74[index] = 0;
    }
    format_calls = 0;
    format_text = 0;
    format_first = 0;
    format_second = 0;
    open_calls = 0;
    open_path = 0;
    open_mode = 0;
    open_result = 1;
    write_calls = 0;
    close_calls = 0;

    DAT_00007edc[0] = 'B';
    DAT_00007edc[1] = 'A';
    DAT_00007edc[2] = 'S';
    DAT_00007edc[3] = 'E';
    DAT_00007edc[4] = 0;

    DAT_00018e5c = (int)replay_frames;
    DAT_00018e70 = 0;
    DAT_0001d2a0 = 0;
    DAT_0001d2b0 = 0;
    DAT_0001d2b4 = 0;
    DAT_0001d260 = 0;
    DAT_0001d274 = 0;
}

int main(void)
{
    int result;

    reset();
    open_result = 0;
    result = replay_save("R1");
    put(result == 1 && open_calls == 1 && write_calls == 0
        && close_calls == 0 ? '0' : 'x');

    reset();
    DAT_00018e70 = 3;
    DAT_0001d2a0 = 0x1111;
    DAT_0001d2b0 = 0x2222;
    DAT_0001d2b4 = 0x3333;
    DAT_0001d260 = 0x4444;
    DAT_0001d274 = 0x5555;
    result = replay_save("R1");
    put(result == 0
        && format_calls == 1
        && text_equal(format_text, "%s\\replays\\%s.rpl")
        && format_first == (void *)DAT_00007edc
        && text_equal((char *)format_second, "R1")
        && open_calls == 1
        && text_equal(open_mode, "wb")
        && write_calls == 5
        && write_ptr[0] == (void *)DAT_000197d8
        && write_size[0] == 0x1c && write_count[0] == 6
        && write_ptr[1] == (void *)DAT_000192a4
        && write_size[1] == 0x1c && write_count[1] == 0x28
        && write_ptr[2] == (void *)DAT_00019704
        && write_size[2] == 4 && write_count[2] == 0xa
        && write_ptr[3] == (void *)DAT_00018e74
        && write_size[3] == 0x18 && write_count[3] == 1
        && write_ptr[4] == (void *)replay_frames
        && write_size[4] == 0x14 && write_count[4] == 3
        && close_calls == 1
        && DAT_00018e74[0] == 3
        && DAT_00018e74[1] == 0x1111
        && DAT_00018e74[2] == 0x2222
        && DAT_00018e74[3] == 0x3333
        && DAT_00018e74[4] == 0x4444
        && DAT_00018e74[5] == 0x5555
        ? '1' : 'x');

    reset();
    DAT_00018e70 = 0;
    DAT_0001d2a0 = 0x6666;
    DAT_0001d274 = 0x7777;
    result = replay_save("R2");
    put(result == 0
        && write_calls == 5
        && write_count[4] == 0
        && write_size[4] == 0x14
        && DAT_00018e74[1] == 0x6666
        && DAT_00018e74[5] == 0x7777
        && close_calls == 1
        ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
