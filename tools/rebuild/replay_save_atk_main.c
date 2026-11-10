/* The test harness for the rebuilt `replay_save_atk` module.
 *
 * The harness holds the base path, the replay name, the 0x24-byte ghost
 * records, the index DAT_0018ed4, the flag cell DAT_001d274, and the zero float
 * DAT_00100a8.  The stubs record the `sprintf`, `fopen`, `fwrite`, and `fclose`
 * arguments, and `fwrite` copies the written bytes into a simulated member
 * image.
 *
 * The harness runs five paths:
 *
 *     0  the open fails: the function returns 0x16 and writes nothing
 *     1  slot 0 with the active flag set: the header and eleven keyframes write
 *     2  the active flag is clear: only the header writes
 *     3  the frame count is zero: the keyframe write carries count 0
 *     4  slot 1: the header comes from the second ghost record
 *     D  the exit marker
 *
 * The harness sets the base path text in the data buffer DAT_00007edc.
 */
#pragma off (check_stack)

#include <stdarg.h>

int replay_save_atk(char *name);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define RECORD_SIZE 0x24
#define RECORD_COUNT 4
#define FRAME_SIZE 0x28
#define MAX_FRAMES 24

unsigned char DAT_00007edc[0x40];
int DAT_0018e8c[(RECORD_SIZE * RECORD_COUNT) / 4];
int DAT_0018ed4;
int DAT_001d274;
float DAT_00100a8;

static unsigned char frame_data[MAX_FRAMES][FRAME_SIZE];
static unsigned char sim_file[RECORD_SIZE + MAX_FRAMES * FRAME_SIZE];

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
static void *write_ptr[8];
static unsigned int write_size[8];
static unsigned int write_count[8];
static int close_calls;
static unsigned int sim_pos;

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
    unsigned int total = size * count;
    unsigned int index;
    const unsigned char *source = (const unsigned char *)ptr;

    if (write_calls < 8) {
        write_ptr[write_calls] = (void *)ptr;
        write_size[write_calls] = size;
        write_count[write_calls] = count;
    }
    write_calls++;
    for (index = 0; index < total; index++) {
        sim_file[sim_pos + index] = source[index];
    }
    sim_pos += total;
    return count;
}

int fclose(void *stream)
{
    close_calls++;
    return 0;
}

static int *record_at(int index)
{
    return (int *)((unsigned char *)DAT_0018e8c + index * RECORD_SIZE);
}

static void reset(void)
{
    int index;

    for (index = 0; index < (int)sizeof(sim_file); index++) {
        sim_file[index] = 0;
    }
    for (index = 0; index < (int)sizeof(frame_data); index++) {
        ((unsigned char *)frame_data)[index] = 0;
    }
    for (index = 0; index < (int)sizeof(DAT_0018e8c); index++) {
        ((unsigned char *)DAT_0018e8c)[index] = 0;
    }
    for (index = 0; index < RECORD_COUNT; index++) {
        record_at(index)[8] = (int)frame_data[index];
    }
    sim_pos = 0;
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
    DAT_0018ed4 = 0;
    DAT_001d274 = 0;
    DAT_00100a8 = 0.0f;

    DAT_00007edc[0] = 'B';
    DAT_00007edc[1] = 'A';
    DAT_00007edc[2] = 'S';
    DAT_00007edc[3] = 'E';
    DAT_00007edc[4] = 0;
}

int main(void)
{
    int result;

    reset();
    open_result = 0;
    result = replay_save_atk("A1");
    put(result == 0x16 && open_calls == 1 && write_calls == 0
        && close_calls == 0 ? '0' : 'x');

    reset();
    DAT_0018ed4 = 0;
    DAT_001d274 = 0x5555;
    record_at(0)[1] = 0x11;
    record_at(0)[3] = 1;
    record_at(0)[0] = 5;
    result = replay_save_atk("A1");
    put(result == 0
        && format_calls == 1
        && text_equal(format_text, "%s\\replays\\%s.atk")
        && format_first == (void *)DAT_00007edc
        && text_equal((char *)format_second, "A1")
        && open_calls == 1
        && text_equal(open_mode, "wb")
        && write_calls == 2
        && write_ptr[0] == (void *)record_at(0)
        && write_size[0] == RECORD_SIZE && write_count[0] == 1
        && write_ptr[1] == (void *)frame_data[0]
        && write_size[1] == FRAME_SIZE && write_count[1] == 0x11
        && record_at(0)[2] == 0x11
        && record_at(0)[4] == 0
        && record_at(0)[5] == 0x5555
        && record_at(0)[7] == 0
        && close_calls == 1
        ? '1' : 'x');

    reset();
    DAT_0018ed4 = 0;
    record_at(0)[1] = 0x22;
    record_at(0)[3] = 0;
    result = replay_save_atk("A2");
    put(result == 0
        && write_calls == 1
        && write_size[0] == RECORD_SIZE && write_count[0] == 1
        && record_at(0)[2] == 0x22
        && close_calls == 1
        ? '2' : 'x');

    reset();
    DAT_0018ed4 = 0;
    record_at(0)[1] = 0;
    record_at(0)[3] = 1;
    result = replay_save_atk("A3");
    put(result == 0
        && write_calls == 2
        && write_count[1] == 0
        && close_calls == 1
        ? '3' : 'x');

    reset();
    DAT_0018ed4 = 1;
    DAT_001d274 = 0x7777;
    record_at(1)[1] = 0;
    record_at(1)[3] = 1;
    result = replay_save_atk("A4");
    put(result == 0
        && write_calls == 2
        && write_ptr[0] == (void *)record_at(1)
        && record_at(1)[2] == 0
        && record_at(1)[4] == 1
        && record_at(1)[5] == 0x7777
        && record_at(0)[2] == 0
        && close_calls == 1
        ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
