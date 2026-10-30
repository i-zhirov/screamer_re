/* The test harness for the rebuilt `dos_memory_alloc` module.
 *
 * The harness holds the two DPMI stubs.  The fill stub records its segment
 * pointer.  The call stub records its arguments and writes the segment, the
 * selector, and the status into the result block.  The harness runs the success
 * path and the failure path:
 *
 *     0  the success path requested 0x13 paragraphs, returned segment * 16, and
 *        stored the selector
 *     1  a non-zero status returns 0 and leaves the selector
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int dos_memory_alloc(int handle, unsigned int size, unsigned int *selector);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static void *filled_segments;
static int got_vector;
static unsigned int *got_regs;
static unsigned int *got_result;
static unsigned int *got_segments;
static int fill_calls;
static int call_calls;
static unsigned int result_segment;
static unsigned int result_selector;
static unsigned int result_status;
static unsigned int request_word0;
static unsigned int request_word1;

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
    unsigned int *words;

    got_vector = vector;
    got_regs = regs;
    got_result = result;
    got_segments = segments;
    call_calls = call_calls + 1;
    request_word0 = got_regs[0];
    request_word1 = got_regs[1];
    words = result;
    words[0] = result_segment;
    words[3] = result_selector;
    words[6] = result_status;
    return 0;
}

int main(void)
{
    unsigned int selector;
    int linear;

    fill_calls = 0;
    call_calls = 0;
    result_segment = 0x1234;
    result_selector = 0x00a7;
    result_status = 0;
    selector = 0xdeadbeef;
    linear = dos_memory_alloc(0, 0x123, &selector);
    put(fill_calls == 1 && call_calls == 1 && got_vector == 0x31
        && got_regs == got_result && got_segments == filled_segments
        && request_word0 == 0x100 && request_word1 == 0x13
        && linear == 0x12340 && selector == 0xa7 ? '0' : 'x');

    fill_calls = 0;
    call_calls = 0;
    result_status = 1;
    selector = 0xdeadbeef;
    linear = dos_memory_alloc(0, 0x123, &selector);
    put(fill_calls == 1 && call_calls == 1
        && linear == 0 && selector == 0xdeadbeef ? '1' : 'x');

    put('D');
    quit();
    return 0;
}
