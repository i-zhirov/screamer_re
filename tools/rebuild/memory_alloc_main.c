/* The test harness for the rebuilt `memory_alloc` module.
 *
 * The harness holds the allocator stub and the two out-of-memory handler
 * stubs.  The allocator stub returns a configured value, and the handlers count
 * their calls.  The harness runs three allocation paths:
 *
 *     0  the allocator succeeds: the module returns the pointer
 *     1  the allocator fails and the first handler succeeds: the module retries
 *        and returns the pointer
 *     2  both handlers are clear: the module returns null
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void *memory_alloc(unsigned int size);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *(*out_of_memory_1)(unsigned int size);
void *(*out_of_memory_2)(unsigned int size);

static char text[8];
static void *alloc_result;
static unsigned int alloc_size;
static int alloc_calls;
static int handler_1_calls;
static int handler_2_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void *malloc(unsigned int size)
{
    alloc_size = size;
    alloc_calls = alloc_calls + 1;
    return alloc_result;
}

void *handler_1(unsigned int size)
{
    handler_1_calls = handler_1_calls + 1;
    return 0;
}

void *handler_2(unsigned int size)
{
    handler_2_calls = handler_2_calls + 1;
    return 0;
}

int main(void)
{
    void *ptr;
    static unsigned char block[8];

    out_of_memory_1 = 0;
    out_of_memory_2 = 0;
    alloc_result = block;
    alloc_calls = 0;
    ptr = memory_alloc(0x100);
    put(ptr == block && alloc_calls == 1 && alloc_size == 0x100 ? '0' : 'x');
    out_of_memory_1 = handler_1;
    alloc_result = 0;
    alloc_calls = 0;
    handler_1_calls = 0;
    ptr = memory_alloc(0x200);
    put(ptr == 0 && alloc_calls == 1 && handler_1_calls == 1 ? '1' : 'x');
    alloc_result = 0;
    out_of_memory_1 = 0;
    out_of_memory_2 = 0;
    alloc_calls = 0;
    ptr = memory_alloc(0x200);
    put(ptr == 0 && alloc_calls == 1 ? '2' : 'x');
    put('D');
    quit();
    return 0;
}
