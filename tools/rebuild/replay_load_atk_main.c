/* The test harness for the rebuilt `replay_load_atk` module.
 *
 * The harness holds the base path, the replay name, the 0x24-byte ghost
 * records, the index DAT_0018ed4, the two exported globals, and a simulated
 * member image.  The stubs record the `sprintf`, `fopen`, `fread`, and
 * `fclose` arguments, and `fread` copies from the simulated image at a running
 * offset.
 *
 * The harness runs six paths:
 *
 *     0  the open fails: the function returns 0x16 and reads nothing
 *     1  slot 0: the header and three keyframes load into record 0
 *     2  slot 1: the header and two keyframes load into record 1
 *     3  the active flag is clear: only the header is read
 *     4  the record count is not above the frame count: the active flag clears
 *     5  the frame count is zero: the keyframe read carries count 0
 *     D  the exit marker
 *
 * The harness sets the base path text in the data buffer DAT_00007edc.
 */
#pragma off (check_stack)

#include <stdarg.h>

int replay_load_atk(char *name);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define HEADER_SIZE 0x24
#define RECORD_SIZE 0x24
#define RECORD_COUNT 4
#define FRAME_SIZE 0x28
#define MAX_FRAMES 8

unsigned char DAT_00007edc[0x40];
int DAT_0018e8c[(RECORD_SIZE * RECORD_COUNT) / 4];
int DAT_0018ed4;
int DAT_001d274;
int DAT_001d26c;

static unsigned char frame_data[MAX_FRAMES][FRAME_SIZE];
static unsigned char sim_file[HEADER_SIZE + MAX_FRAMES * FRAME_SIZE];

static char text[8];

static int format_calls;
static char *format_text;
static void *format_first;
static void *format_second;
static int open_calls;
static char *open_path;
static char *open_mode;
static int open_result;
static int read_calls;
static void *read_ptr[8];
static unsigned int read_size[8];
static unsigned int read_count[8];
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

unsigned int fread(void *ptr, unsigned int size, unsigned int count,
    void *stream)
{
    unsigned int total = size * count;
    unsigned int index;
    unsigned char *dest = (unsigned char *)ptr;

    if (read_calls < 8) {
        read_ptr[read_calls] = ptr;
        read_size[read_calls] = size;
        read_count[read_calls] = count;
    }
    read_calls++;
    for (index = 0; index < total; index++) {
        dest[index] = sim_file[sim_pos + index];
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

static void set_header(int slot, int frame_count, int active, int field18,
    int clock)
{
    int *header = (int *)sim_file;

    header[0] = 0;
    header[1] = 0;
    header[2] = frame_count;
    header[3] = active;
    header[4] = slot;
    header[5] = 0;
    header[6] = field18;
    header[7] = clock;
    header[8] = 0;
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
        record_at(index)[8] = (int)frame_data;
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
    read_calls = 0;
    close_calls = 0;
    DAT_0018ed4 = 0;
    DAT_001d274 = 0;
    DAT_001d26c = 0;

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
    result = replay_load_atk("R1");
    put(result == 0x16 && open_calls == 1 && read_calls == 0
        && close_calls == 0 ? '0' : 'x');

    reset();
    set_header(0, 3, 1, 0x3333, 0x4444);
    sim_file[HEADER_SIZE] = 0x7a;
    record_at(0)[0] = 5;
    record_at(0)[5] = 0x5555;
    result = replay_load_atk("R1");
    put(result == 0
        && format_calls == 1
        && text_equal(format_text, "%s\\replays\\%s.atk")
        && format_first == (void *)DAT_00007edc
        && text_equal((char *)format_second, "R1")
        && open_calls == 1
        && text_equal(open_mode, "rb")
        && read_calls == 2
        && read_size[0] == 1 && read_count[0] == 0x24
        && read_ptr[1] == (void *)frame_data
        && read_size[1] == FRAME_SIZE && read_count[1] == 3
        && frame_data[0][0] == 0x7a
        && close_calls == 1
        && record_at(0)[2] == 3
        && record_at(0)[3] == 1
        && record_at(0)[6] == 0x3333
        && record_at(0)[7] == 0x4444
        && record_at(0)[0] == 5
        && DAT_001d274 == 0x5555
        && DAT_001d26c == 0x4444
        && (DAT_0018ed4 & 0xff) == 1
        ? '1' : 'x');

    reset();
    set_header(1, 2, 1, 0x7777, 0x8888);
    record_at(1)[0] = 5;
    record_at(1)[5] = 0x9999;
    result = replay_load_atk("R2");
    put(result == 0
        && read_calls == 2
        && read_ptr[1] == (void *)frame_data
        && read_count[1] == 2
        && record_at(1)[2] == 2
        && record_at(1)[3] == 1
        && record_at(1)[6] == 0x7777
        && record_at(1)[7] == 0x8888
        && record_at(0)[2] == 0
        && DAT_001d274 == 0x9999
        && DAT_001d26c == 0x8888
        && (DAT_0018ed4 & 0xff) == 0
        ? '2' : 'x');

    reset();
    set_header(1, 3, 0, 0x1111, 0x2222);
    record_at(1)[0] = 5;
    result = replay_load_atk("R2");
    put(result == 0
        && read_calls == 1
        && record_at(1)[2] == 3
        && record_at(1)[3] == 0
        && record_at(1)[6] == 0x1111
        && record_at(1)[7] == 0x2222
        && close_calls == 1
        ? '3' : 'x');

    reset();
    set_header(0, 3, 1, 0xaaaa, 0xbbbb);
    record_at(0)[0] = 2;
    result = replay_load_atk("R1");
    put(result == 0
        && read_calls == 1
        && record_at(0)[3] == 0
        && record_at(0)[2] == 3
        && close_calls == 1
        ? '4' : 'x');

    reset();
    set_header(0, 0, 1, 0xcccc, 0xdddd);
    record_at(0)[0] = 5;
    result = replay_load_atk("R1");
    put(result == 0
        && read_calls == 2
        && read_count[1] == 0
        && record_at(0)[3] == 1
        && close_calls == 1
        ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
