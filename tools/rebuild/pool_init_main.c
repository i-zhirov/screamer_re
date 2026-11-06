/* The test harness for the rebuilt `pool_init` module.
 *
 * The harness holds the pool record and the two DPMI stubs.  The call stub
 * captures the request word 5 before it writes the result.  The harness runs
 * the success path with a positive field, the success path with a zero field,
 * and the status failure path:
 *
 *     0  the positive field converts to bytes from 4 KB pages
 *     1  the zero field stays 0
 *     2  a non-zero status returns 1 and leaves the fields
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int pool_init(char *pool);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char pool[0x50];
static void *filled_segments;
static int got_vector;
static unsigned int *got_regs;
static unsigned int *got_result;
static unsigned int *got_segments;
static unsigned int got_buffer;
static unsigned int pages_locked;
static unsigned int pages_free;
static unsigned int result_status;
static int fill_calls;
static int call_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void dpmi_fill_regs(void *segments)
{
    filled_segments = segments;
    fill_calls = fill_calls + 1;
}

int dpmi_call(int vector, void *regs, void *result, void *segments)
{
    unsigned int *input;
    unsigned int *output;
    unsigned int *buffer;

    input = regs;
    got_vector = vector;
    got_regs = regs;
    got_result = result;
    got_segments = segments;
    got_buffer = input[5];
    buffer = (unsigned int *)input[5];
    call_calls = call_calls + 1;
    if (result_status == 0) {
        buffer[2] = pages_locked;
        buffer[8] = pages_free;
    }
    output = result;
    output[6] = result_status;
    return 0;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x50; ++index) {
        pool[index] = 0;
    }
    fill_calls = 0;
    call_calls = 0;
    got_vector = 0;
    got_regs = 0;
    got_result = 0;
    got_segments = 0;
    got_buffer = 0;
    pages_locked = 0;
    pages_free = 0;
    result_status = 0;
}

int main(void)
{
    int result;

    reset();
    pages_locked = 0x10;
    pages_free = 0x20;
    *(unsigned int *)(pool + 0x24) = 0;
    *(unsigned int *)(pool + 0x3c) = 0;
    result = pool_init(pool);
    put(result == 0 && fill_calls == 1 && call_calls == 1 && got_vector == 0x31
        && got_regs == got_result && got_segments == filled_segments
        && got_regs[0] == 0x500 && got_buffer == (unsigned int)(pool + 0x1c)
        && *(unsigned int *)(pool + 0x24) == 0x10000
        && *(unsigned int *)(pool + 0x3c) == 0x20000 ? '0' : 'x');

    reset();
    pages_locked = 1;
    pages_free = 0;
    *(unsigned int *)(pool + 0x24) = 0;
    *(unsigned int *)(pool + 0x3c) = 0;
    result = pool_init(pool);
    put(result == 0 && *(unsigned int *)(pool + 0x24) == 0x1000
        && *(unsigned int *)(pool + 0x3c) == 0 ? '1' : 'x');

    reset();
    result_status = 1;
    *(unsigned int *)(pool + 0x24) = 0x1234;
    *(unsigned int *)(pool + 0x3c) = 0x5678;
    result = pool_init(pool);
    put(result == 1 && *(unsigned int *)(pool + 0x24) == 0x1234
        && *(unsigned int *)(pool + 0x3c) == 0x5678 ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
