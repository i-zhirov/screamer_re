/* The test harness for the rebuilt `replay_load` module.
 *
 * The harness holds the base path, the replay name, the four header blocks,
 * the frame buffer pointer, the frame limit, the header cells, and a simulated
 * member image.  The stubs record the `sprintf`, `fopen`, `fread`, and `fclose`
 * arguments, and `fread` copies from the simulated image at a running offset.
 *
 * The harness runs five paths:
 *
 *     0  the open fails: the function returns 1 and reads nothing
 *     1  the flag is set: four header reads, the header copy, and three frame
 *        records
 *     2  the flag is clear: four header reads, the header copy, no frame read
 *     3  the frame buffer is null: the function returns 1 after the headers
 *     4  the frame count is at the limit: the function returns 1 after the
 *        headers
 *     D  the exit marker
 *
 * The harness sets the base path text in the data buffer DAT_00007edc.
 */
#pragma off (check_stack)

#include <stdarg.h>

int replay_load(char *name, int load_frames);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define BLOCK1_SIZE 0xa8
#define BLOCK2_SIZE 0x460
#define BLOCK3_SIZE 0x28
#define HEADER_SIZE 0x18
#define HEADER_OFFSET (BLOCK1_SIZE + BLOCK2_SIZE + BLOCK3_SIZE)
#define FRAME_OFFSET (HEADER_OFFSET + HEADER_SIZE)
#define FRAME_SIZE 0x14
#define MAX_FRAMES 8

unsigned char DAT_00007edc[0x40];
int DAT_00018e5c;
int DAT_00018e68;
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
static unsigned char sim_file[FRAME_OFFSET + MAX_FRAMES * FRAME_SIZE];

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

static void set_header(int count, int first, int second, int third,
    int fourth, int fifth)
{
    int *header = (int *)(sim_file + HEADER_OFFSET);

    header[0] = count;
    header[1] = first;
    header[2] = second;
    header[3] = third;
    header[4] = fourth;
    header[5] = fifth;
}

static void reset(void)
{
    int index;

    for (index = 0; index < (int)sizeof(sim_file); index++) {
        sim_file[index] = 0;
    }
    for (index = 0; index < (int)sizeof(replay_frames); index++) {
        ((unsigned char *)replay_frames)[index] = 0;
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

    DAT_00007edc[0] = 'B';
    DAT_00007edc[1] = 'A';
    DAT_00007edc[2] = 'S';
    DAT_00007edc[3] = 'E';
    DAT_00007edc[4] = 0;

    DAT_00018e5c = (int)replay_frames;
    DAT_00018e68 = 0x40;
    DAT_00018e70 = 0;
    DAT_0001d2a0 = 0;
    DAT_0001d2b0 = 0;
    DAT_0001d2b4 = 0;
    DAT_0001d260 = 0;
    DAT_0001d274 = 0;
    set_header(0, 0, 0, 0, 0, 0);
}

int main(void)
{
    int result;

    reset();
    open_result = 0;
    result = replay_load("R1", 1);
    put(result == 1 && open_calls == 1 && read_calls == 0 && close_calls == 0
        ? '0' : 'x');

    reset();
    set_header(3, 0x1111, 0x2222, 0x3333, 0x4444, 0x5555);
    result = replay_load("R1", 1);
    put(result == 0
        && format_calls == 1
        && text_equal(format_text, "%s\\replays\\%s.rpl")
        && format_first == (void *)DAT_00007edc
        && text_equal((char *)format_second, "R1")
        && open_calls == 1
        && text_equal(open_mode, "rb")
        && read_calls == 5
        && read_ptr[0] == (void *)DAT_000197d8
        && read_size[0] == 0x1c && read_count[0] == 6
        && read_ptr[1] == (void *)DAT_000192a4
        && read_size[1] == 0x1c && read_count[1] == 0x28
        && read_ptr[2] == (void *)DAT_00019704
        && read_size[2] == 4 && read_count[2] == 0xa
        && read_ptr[3] == (void *)DAT_00018e74
        && read_size[3] == 0x18 && read_count[3] == 1
        && read_ptr[4] == (void *)replay_frames
        && read_size[4] == 0x14 && read_count[4] == 3
        && close_calls == 1
        && DAT_00018e70 == 3
        && DAT_0001d2a0 == 0x1111
        && DAT_0001d2b0 == 0x2222
        && DAT_0001d2b4 == 0x3333
        && DAT_0001d260 == 0x4444
        && DAT_0001d274 == 0x5555
        ? '1' : 'x');

    reset();
    set_header(3, 0x1111, 0x2222, 0x3333, 0x4444, 0x5555);
    result = replay_load("R1", 0);
    put(result == 0 && read_calls == 4 && close_calls == 1
        && DAT_00018e70 == 3 && DAT_0001d260 == 0x4444 ? '2' : 'x');

    reset();
    set_header(3, 0x1111, 0x2222, 0x3333, 0x4444, 0x5555);
    DAT_00018e5c = 0;
    result = replay_load("R1", 1);
    put(result == 1 && read_calls == 4 && close_calls == 1 ? '3' : 'x');

    reset();
    set_header(5, 0x1111, 0x2222, 0x3333, 0x4444, 0x5555);
    DAT_00018e68 = 5;
    result = replay_load("R1", 1);
    put(result == 1 && read_calls == 4 && close_calls == 1 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
