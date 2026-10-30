/* The test harness for the rebuilt `list_create` module.
 *
 * The harness holds the list record and the three stubs.  The stubs record
 * their arguments.  The harness runs the success path and the allocation
 * failure path:
 *
 *     0  the success path stores the pointer, the cursor 0, and the count 3,
 *        and zeroes 0x54 bytes
 *     1  the failure path stores the pointer 0, the cursor 0, and the count 0
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void *list_create(void *list, int count);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char list[0x30];
static unsigned char block[0x60];
static void *malloc_result;
static void *pool_list;
static unsigned int malloc_size;
static void *memset_block;
static int memset_value;
static unsigned int memset_size;
static int pool_calls;
static int malloc_calls;
static int memset_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void pool_init(void *target)
{
    pool_list = target;
    pool_calls = pool_calls + 1;
}

void *malloc(unsigned int size)
{
    malloc_size = size;
    malloc_calls = malloc_calls + 1;
    return malloc_result;
}

void *memset(void *target, int value, unsigned int size)
{
    memset_block = target;
    memset_value = value;
    memset_size = size;
    memset_calls = memset_calls + 1;
    return target;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x30; ++index) {
        list[index] = 0xaa;
    }
    pool_calls = 0;
    malloc_calls = 0;
    memset_calls = 0;
    pool_list = 0;
    malloc_size = 0;
    memset_block = 0;
    memset_value = -1;
    memset_size = 0;
}

int main(void)
{
    void *result;

    reset();
    malloc_result = block;
    result = list_create(list, 3);
    put(result == list && pool_calls == 1 && pool_list == list
        && malloc_calls == 1 && malloc_size == 0x54
        && *(void **)(list + 0) == block
        && *(int *)(list + 4) == 0 && *(int *)(list + 8) == 3
        && memset_calls == 1 && memset_block == block
        && memset_value == 0 && memset_size == 0x54 ? '0' : 'x');

    reset();
    malloc_result = 0;
    result = list_create(list, 3);
    put(result == list && pool_calls == 1 && malloc_calls == 1
        && *(void **)(list + 0) == 0
        && *(int *)(list + 4) == 0 && *(int *)(list + 8) == 0
        && memset_calls == 0 ? '1' : 'x');

    put('D');
    quit();
    return 0;
}
