/* The test harness for the rebuilt `memory_block_largest` module.
 *
 * The harness holds the two scripted sizes, the two recorded pool pointers, and
 * the two callee stubs.  Each stub records its pool argument and its call count
 * and returns its scripted size.
 *
 * The harness runs five paths:
 *
 *     0  the DOS size 100 and the heap size 200: the heap block wins, and both
 *        stubs receive the same pool pointer
 *     1  the DOS size 300 and the heap size 200: the DOS block wins
 *     2  the two sizes 150: the DOS block wins on the equal compare
 *     3  the DOS size -1 and the heap size 0: the signed compare keeps the heap
 *        block
 *     4  the DOS size 0 and the heap size -5: the signed compare keeps the DOS
 *        block
 *     D  the exit marker
 */
#pragma off (check_stack)

int memory_block_largest(void *pool);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char pool_image[16];
static int dos_size;
static int heap_size;
static void *dos_pool;
static void *heap_pool;
static int dos_calls;
static int heap_calls;

int dos_block_largest(void *pool)
{
    dos_calls++;
    dos_pool = pool;
    return dos_size;
}

int heap_block_largest(void *pool)
{
    heap_calls++;
    heap_pool = pool;
    return heap_size;
}

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void check(int number, int ok)
{
    put(ok ? (char)('0' + number) : 'x');
}

static void reset(void)
{
    dos_size = 0;
    heap_size = 0;
    dos_pool = 0;
    heap_pool = 0;
    dos_calls = 0;
    heap_calls = 0;
}

int main(void)
{
    int result;

    reset();
    dos_size = 100;
    heap_size = 200;
    result = memory_block_largest(pool_image);
    check(0, result == 200
        && dos_calls == 1
        && heap_calls == 1
        && dos_pool == pool_image
        && heap_pool == pool_image);

    reset();
    dos_size = 300;
    heap_size = 200;
    result = memory_block_largest(pool_image);
    check(1, result == 300
        && dos_calls == 1
        && heap_calls == 1);

    reset();
    dos_size = 150;
    heap_size = 150;
    result = memory_block_largest(pool_image);
    check(2, result == 150);

    reset();
    dos_size = -1;
    heap_size = 0;
    result = memory_block_largest(pool_image);
    check(3, result == 0
        && dos_calls == 1
        && heap_calls == 1);

    reset();
    dos_size = 0;
    heap_size = -5;
    result = memory_block_largest(pool_image);
    check(4, result == 0);

    put('D');
    quit();
    return 0;
}
