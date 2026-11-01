/* The test harness for the rebuilt `memory_alloc` module at 0x2f172.
 *
 * The harness holds the two DPMI stubs.  The fill stub records its segment
 * pointer.  The call stub records the arguments and the three request words,
 * and writes the address high word, the address low word, and the status into
 * the result block.  The harness runs the success path and the failure path:
 *
 *     0  the success path requested 0x0501 with the size words 0x1234 and
 *        0x5678, returned the size, and stored the address 0x9abcdef0
 *     1  a non-zero status returns 0 and leaves the address
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int memory_alloc_small(int handle, int size, unsigned int *address);
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
static unsigned int request_word0;
static unsigned int request_word1;
static unsigned int request_word2;
static unsigned int result_address_high;
static unsigned int result_address_low;
static unsigned int result_status;

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
    request_word2 = got_regs[2];
    words = result;
    words[4] = result_address_high;
    words[5] = result_address_low;
    words[6] = result_status;
    return 0;
}

int main(void)
{
    unsigned int address;
    int result;

    fill_calls = 0;
    call_calls = 0;
    result_address_high = 0x9abc;
    result_address_low = 0xdef0;
    result_status = 0;
    address = 0xdeadbeef;
    result = memory_alloc_small(0, 0x12345678, &address);
    put(fill_calls == 1 && call_calls == 1 && got_vector == 0x31
        && got_regs == got_result && got_segments == filled_segments
        && request_word0 == 0x501 && request_word1 == 0x1234
        && request_word2 == 0x5678
        && result == 0x12345678 && address == 0x9abcdef0 ? '0' : 'x');

    fill_calls = 0;
    call_calls = 0;
    result_status = 1;
    address = 0xdeadbeef;
    result = memory_alloc_small(0, 0x12345678, &address);
    put(fill_calls == 1 && call_calls == 1
        && result == 0 && address == 0xdeadbeef ? '1' : 'x');

    put('D');
    quit();
    return 0;
}
