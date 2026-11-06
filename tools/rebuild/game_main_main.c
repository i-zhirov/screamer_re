/* The test harness for the rebuilt `main` module.
 *
 * The harness holds the two globals and the thirteen stubs.  Each stub records
 * its arguments and its call count.  The harness runs four paths:
 *
 *     0  a low memory value reaches the two messages, the shortfall, and the
 *        cleanup of the list block and the list record
 *     1  the normal path runs the engine init, three race loop turns, three
 *        frame updates, the engine shutdown, and the cleanup
 *     2  a non-zero race result calls the video mode 3, the error report, and
 *        the exit with the same code, and then the loop ends through the quit
 *        flag that the exit stub sets
 *     3  a failed allocation skips the list, still reads the memory twice, and
 *        skips the cleanup
 *     D  the exit marker
 *
 * The exit stub returns, because the harness must continue after the error
 * path.  It sets the quit flag, so the loop ends on its next test.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)
#include <stdarg.h>

void game_main(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_000062ac;
unsigned int DAT_00007ed8;

static char text[8];
static char block[0x200];
static char msg_low[] = "\nNot enough memory to play SCREAMER\n";
static char msg_need[] = "You need another %d bytes\n";

static unsigned int alloc_size;
static void *alloc_result;
static int create_calls;
static void *create_list;
static int create_count;
static int memory_calls;
static void *memory_pool[2];
static unsigned int available;
static int print_calls;
static char *got_format[2];
static int got_value;
static int init_calls;
static int race_calls;
static int race_result;
static int video_calls;
static int video_mode;
static int report_calls;
static int report_code;
static int exit_calls;
static int exit_code;
static int exit_sets_quit;
static int frame_calls;
static int frame_quit_after;
static int shutdown_calls;
static int destroy_calls;
static void *destroy_list;
static int destroy_unused;
static int free_calls;
static void *freed;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int same(char *first, char *second)
{
    while (*first != 0) {
        if (*first != *second) {
            return 0;
        }
        ++first;
        ++second;
    }
    return *second == 0;
}

void *memory_alloc(unsigned int size)
{
    alloc_size = size;
    return alloc_result;
}

void *list_create(void *list, int count)
{
    create_calls = create_calls + 1;
    create_list = list;
    create_count = count;
    return list;
}

unsigned int memory_available(void *pool)
{
    if (memory_calls < 2) {
        memory_pool[memory_calls] = pool;
    }
    memory_calls = memory_calls + 1;
    return available;
}

int printf(const char *format, ...)
{
    va_list arguments;

    if (print_calls < 2) {
        got_format[print_calls] = (char *)format;
    }
    va_start(arguments, format);
    if (print_calls == 1) {
        got_value = va_arg(arguments, int);
    }
    va_end(arguments);
    print_calls = print_calls + 1;
    return 0;
}

void engine_init(void)
{
    init_calls = init_calls + 1;
}

int race_loop(void)
{
    race_calls = race_calls + 1;
    return race_result;
}

void bios_video(int mode)
{
    video_calls = video_calls + 1;
    video_mode = mode;
}

void error_report(int code)
{
    report_calls = report_calls + 1;
    report_code = code;
}

void exit_proc(int code)
{
    exit_calls = exit_calls + 1;
    exit_code = code;
    if (exit_sets_quit != 0) {
        DAT_00007ed8 = 1;
    }
}

void frame_update(void)
{
    frame_calls = frame_calls + 1;
    if (frame_quit_after != 0 && frame_calls >= frame_quit_after) {
        DAT_00007ed8 = 1;
    }
}

void engine_shutdown(void)
{
    shutdown_calls = shutdown_calls + 1;
}

void *list_destroy(void *list, int unused)
{
    destroy_calls = destroy_calls + 1;
    destroy_list = list;
    destroy_unused = unused;
    return list;
}

void release(void *block_pointer)
{
    free_calls = free_calls + 1;
    freed = block_pointer;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x200; ++index) {
        block[index] = 0;
    }
    alloc_size = 0;
    alloc_result = 0;
    create_calls = 0;
    create_list = 0;
    create_count = 0;
    memory_calls = 0;
    memory_pool[0] = 0;
    memory_pool[1] = 0;
    available = 0;
    print_calls = 0;
    got_format[0] = 0;
    got_format[1] = 0;
    got_value = 0;
    init_calls = 0;
    race_calls = 0;
    race_result = 0;
    video_calls = 0;
    video_mode = 0;
    report_calls = 0;
    report_code = 0;
    exit_calls = 0;
    exit_code = 0;
    exit_sets_quit = 0;
    frame_calls = 0;
    frame_quit_after = 0;
    shutdown_calls = 0;
    destroy_calls = 0;
    destroy_list = 0;
    destroy_unused = 0;
    free_calls = 0;
    freed = 0;
    DAT_000062ac = 0;
    DAT_00007ed8 = 0;
}

int main(void)
{
    reset();
    alloc_result = block;
    available = 0x1000;
    game_main();
    put(alloc_size == 0x4c && create_calls == 1 && create_list == block
        && create_count == 0x96 && DAT_000062ac == block
        && memory_calls == 2 && memory_pool[0] == block
        && memory_pool[1] == block
        && print_calls == 2 && same(got_format[0], msg_low)
        && same(got_format[1], msg_need) && got_value == 0x619800
        && init_calls == 0 && race_calls == 0 && video_calls == 0
        && report_calls == 0 && exit_calls == 0 && frame_calls == 0
        && shutdown_calls == 0 && destroy_calls == 1
        && destroy_list == block && destroy_unused == 0
        && free_calls == 1 && freed == block ? '0' : 'x');

    reset();
    alloc_result = block;
    available = 0x61a800;
    race_result = 0;
    frame_quit_after = 3;
    game_main();
    put(alloc_size == 0x4c && create_calls == 1 && create_count == 0x96
        && DAT_000062ac == block && memory_calls == 1
        && print_calls == 0 && init_calls == 1 && race_calls == 3
        && frame_calls == 3 && video_calls == 0 && report_calls == 0
        && exit_calls == 0 && shutdown_calls == 1
        && destroy_calls == 1 && free_calls == 1 && freed == block
        ? '1' : 'x');

    reset();
    alloc_result = block;
    available = 0x61a800;
    race_result = 7;
    exit_sets_quit = 1;
    game_main();
    put(alloc_size == 0x4c && create_calls == 1 && init_calls == 1
        && race_calls == 1 && video_calls == 1 && video_mode == 3
        && report_calls == 1 && report_code == 7 && exit_calls == 1
        && exit_code == 7 && frame_calls == 1 && shutdown_calls == 1
        && print_calls == 0 && destroy_calls == 1 && free_calls == 1
        && freed == block ? '2' : 'x');

    reset();
    alloc_result = 0;
    available = 0x1000;
    game_main();
    put(alloc_size == 0x4c && create_calls == 0 && DAT_000062ac == 0
        && memory_calls == 2 && memory_pool[0] == 0 && memory_pool[1] == 0
        && print_calls == 2 && same(got_format[0], msg_low)
        && same(got_format[1], msg_need) && got_value == 0x619800
        && init_calls == 0 && race_calls == 0 && destroy_calls == 0
        && free_calls == 0 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
