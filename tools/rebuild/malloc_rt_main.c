/* The test harness for the rebuilt `malloc` module.
 *
 * The harness holds the four module globals, three free-list blocks, the
 * `heap_alloc` stub, the `heap_grow` stub, and the `heap_expand` stub.  The
 * alloc stub records its arguments and fails the first `alloc_fail_calls`
 * calls.  The grow and expand stubs return their configured results.  The
 * harness runs nine paths:
 *
 *     0  a size at or below the cached largest free size restarts at the head
 *        and clears the cache; the first block succeeds and clears the flag
 *     1  the size 0 returns 0 and leaves the cache and the flag
 *     2  a size above 0xffffffd4 returns 0 and leaves the cache and the flag
 *     3  the first block fails, the second succeeds, and the cache keeps the
 *        largest failed block
 *     4  a size above the cache resumes at the saved cursor
 *     5  every block fails, the grow succeeds, the walk restarts, and the
 *        second walk succeeds
 *     6  every block fails, the grow fails, the expand succeeds, the walk
 *        restarts, and the second walk succeeds
 *     7  every block fails, the grow and the expand fail, and the function
 *        returns 0 and keeps the flag
 *     8  the size 1 rounds up to the minimum 12 and resumes at the cursor
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

struct free_block {
    int field_00;
    int field_04;
    struct free_block *next;
    int field_0c;
    int field_10;
    unsigned int size;
};

void *malloc_rt(unsigned int size);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char marker[4];
static struct free_block blocks[3];
struct free_block *heap_free_list;
struct free_block *heap_cursor;
unsigned int heap_largest_free;
unsigned char heap_flag;

static int alloc_calls;
static int alloc_fail_calls;
static unsigned int alloc_size;
static unsigned int alloc_segment;
static struct free_block *alloc_block;
static void *alloc_result;
static int grow_calls;
static unsigned int grow_size;
static int grow_result;
static int expand_calls;
static unsigned int expand_size;
static int expand_result;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void *heap_alloc(unsigned int size, unsigned int segment,
                 struct free_block *block)
{
    alloc_calls = alloc_calls + 1;
    alloc_size = size;
    alloc_segment = segment;
    alloc_block = block;
    if (alloc_calls <= alloc_fail_calls) {
        return 0;
    }
    return alloc_result;
}

int heap_grow(unsigned int size)
{
    grow_calls = grow_calls + 1;
    grow_size = size;
    return grow_result;
}

int heap_expand(unsigned int size)
{
    expand_calls = expand_calls + 1;
    expand_size = size;
    return expand_result;
}

static void set_block(int index, struct free_block *next, unsigned int size)
{
    blocks[index].field_00 = 0;
    blocks[index].field_04 = 0;
    blocks[index].next = next;
    blocks[index].field_0c = 0;
    blocks[index].field_10 = 0;
    blocks[index].size = size;
}

static void reset(unsigned int largest)
{
    heap_free_list = 0;
    heap_cursor = 0;
    heap_largest_free = largest;
    heap_flag = 1;
    alloc_calls = 0;
    alloc_fail_calls = 0;
    alloc_size = 0;
    alloc_segment = 0xdeadbeef;
    alloc_block = 0;
    alloc_result = marker;
    grow_calls = 0;
    grow_size = 0;
    grow_result = 0;
    expand_calls = 0;
    expand_size = 0;
    expand_result = 0;
    set_block(0, 0, 0);
    set_block(1, 0, 0);
    set_block(2, 0, 0);
}

int main(void)
{
    void *result;

    reset(0x100);
    set_block(0, 0, 0x40);
    heap_free_list = &blocks[0];
    result = malloc_rt(10);
    put(result == marker && alloc_calls == 1 && alloc_size == 10
        && alloc_segment == 0 && alloc_block == &blocks[0]
        && heap_cursor == &blocks[0] && heap_largest_free == 0
        && heap_flag == 0 && grow_calls == 0 && expand_calls == 0 ? '0' : 'x');

    reset(0x100);
    set_block(0, 0, 0x40);
    heap_free_list = &blocks[0];
    heap_flag = 1;
    result = malloc_rt(0);
    put(result == 0 && alloc_calls == 0 && heap_largest_free == 0x100
        && heap_cursor == 0 && heap_flag == 1 ? '1' : 'x');

    reset(0x100);
    set_block(0, 0, 0x40);
    heap_free_list = &blocks[0];
    heap_flag = 1;
    result = malloc_rt(0xffffffd5);
    put(result == 0 && alloc_calls == 0 && heap_largest_free == 0x100
        && heap_cursor == 0 && heap_flag == 1 ? '2' : 'x');

    reset(0);
    set_block(0, &blocks[1], 8);
    set_block(1, 0, 20);
    heap_free_list = &blocks[0];
    alloc_fail_calls = 1;
    result = malloc_rt(16);
    put(result == marker && alloc_calls == 2 && alloc_block == &blocks[1]
        && heap_cursor == &blocks[1] && heap_largest_free == 8
        && heap_flag == 0 ? '3' : 'x');

    reset(0);
    set_block(0, &blocks[1], 8);
    set_block(1, 0, 20);
    heap_free_list = &blocks[0];
    heap_cursor = &blocks[1];
    result = malloc_rt(16);
    put(result == marker && alloc_calls == 1 && alloc_block == &blocks[1]
        && heap_cursor == &blocks[1] && heap_flag == 0 ? '4' : 'x');

    reset(0);
    set_block(0, 0, 100);
    heap_free_list = &blocks[0];
    alloc_fail_calls = 1;
    grow_result = 1;
    result = malloc_rt(16);
    put(result == marker && alloc_calls == 2 && grow_calls == 1
        && grow_size == 16 && expand_calls == 0 && alloc_block == &blocks[0]
        && heap_largest_free == 0 && heap_flag == 0 ? '5' : 'x');

    reset(0);
    set_block(0, 0, 100);
    heap_free_list = &blocks[0];
    alloc_fail_calls = 1;
    grow_result = 0;
    expand_result = 1;
    result = malloc_rt(16);
    put(result == marker && alloc_calls == 2 && grow_calls == 1
        && expand_calls == 1 && expand_size == 16 && alloc_block == &blocks[0]
        && heap_largest_free == 0 && heap_flag == 0 ? '6' : 'x');

    reset(0);
    set_block(0, 0, 100);
    heap_free_list = &blocks[0];
    alloc_fail_calls = 1;
    heap_flag = 1;
    result = malloc_rt(16);
    put(result == 0 && alloc_calls == 1 && grow_calls == 1
        && expand_calls == 1 && heap_largest_free == 100
        && heap_cursor == &blocks[0] && heap_flag == 1 ? '7' : 'x');

    reset(11);
    set_block(0, &blocks[1], 100);
    set_block(1, 0, 100);
    heap_free_list = &blocks[0];
    heap_cursor = &blocks[1];
    result = malloc_rt(1);
    put(result == marker && alloc_calls == 1 && alloc_size == 1
        && alloc_block == &blocks[1] && heap_cursor == &blocks[1] ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
