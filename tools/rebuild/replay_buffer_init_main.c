/* The test harness for the rebuilt `replay_buffer_init` module.
 *
 * The harness holds the two ghost records, the index, the cell `DAT_0018ee4`,
 * the context pointer, and the two callee stubs.  The `memory_block_largest`
 * stub returns a scripted value, and the `resource_get` stub returns a scripted
 * pointer and records its arguments.
 *
 * The harness runs six paths:
 *
 *     0  the count 199 is below 0xc8: the function clears the four cells and
 *        calls no resource
 *     1  the count 200 loads the resource: both records take the half 100 and
 *        the function returns 1
 *     2  the count 200 with a failed load: the count stays and the four cells
 *        are cleared
 *     3  the count 201 loads the resource: the half 100 sits at the record and
 *        at the second pointer offset
 *     4  the count 203 loads the resource: the half 101 confirms the truncation
 *     5  the count 1000 loads the resource: the half 500 confirms the scale
 *     D  the exit marker
 */
#pragma off (check_stack)

int replay_buffer_init(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_000062ac;
char DAT_00100db3[16];
int DAT_0018e8c;
int DAT_0018eac;
int DAT_0018eb0;
int DAT_0018ebc;
int DAT_0018e98;
int DAT_0018ed0;
int DAT_0018ed4;
int DAT_0018ee4;

static char text[8];
static char pool_image[16];

static unsigned int largest;
static void *resource_pointer;
static int resource_calls;
static void *resource_context;
static char *resource_name;
static unsigned int resource_size;

unsigned int memory_block_largest(void *pool)
{
    (void)pool;
    return largest;
}

void *resource_get(void *resource, char *name, unsigned int size)
{
    resource_calls++;
    resource_context = resource;
    resource_name = name;
    resource_size = size;
    return resource_pointer;
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

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

static void reset(void)
{
    DAT_000062ac = pool_image;
    DAT_00100db3[0] = 'A';
    DAT_00100db3[1] = 'T';
    DAT_00100db3[2] = 'T';
    DAT_00100db3[3] = 'A';
    DAT_00100db3[4] = 'C';
    DAT_00100db3[5] = 'K';
    DAT_00100db3[6] = 0;
    DAT_0018e8c = 0x1111;
    DAT_0018eac = 0x2222;
    DAT_0018eb0 = 0x3333;
    DAT_0018ebc = 0x4444;
    DAT_0018e98 = 0x5555;
    DAT_0018ed0 = 0x6666;
    DAT_0018ed4 = 0x7777;
    DAT_0018ee4 = 0x8888;
    largest = 0;
    resource_pointer = pool_image;
    resource_calls = 0;
    resource_context = 0;
    resource_name = 0;
    resource_size = 0;
}

static int cleared(void)
{
    return DAT_0018eac == 0
        && DAT_0018ebc == 0
        && DAT_0018e98 == 0
        && DAT_0018ed0 == 0;
}

static int untouched(void)
{
    return DAT_0018eb0 == 0x3333
        && DAT_0018ed4 == 0x7777
        && DAT_0018ee4 == 0x8888;
}

static int name_is_attack(void)
{
    char *name = resource_name;

    return name != 0
        && name[0] == 'A'
        && name[1] == 'T'
        && name[2] == 'T'
        && name[3] == 'A'
        && name[4] == 'C'
        && name[5] == 'K'
        && name[6] == 0;
}

int main(void)
{
    int result;
    int ok;

    reset();
    largest = 7960;
    result = replay_buffer_init();
    ok = result == 0
        && DAT_0018e8c == 199
        && cleared()
        && untouched()
        && resource_calls == 0;
    report(ok, '0');

    reset();
    largest = 8000;
    result = replay_buffer_init();
    ok = result == 1
        && DAT_0018e8c == 100
        && DAT_0018eb0 == 100
        && DAT_0018ed4 == 0
        && DAT_0018ee4 == 0
        && DAT_0018ebc == 1
        && DAT_0018e98 == 1
        && DAT_0018eac == (int)pool_image
        && DAT_0018ed0 == (int)pool_image + 4000
        && resource_calls == 1
        && resource_context == pool_image
        && resource_size == 8000
        && name_is_attack();
    report(ok, '1');

    reset();
    largest = 8000;
    resource_pointer = 0;
    result = replay_buffer_init();
    ok = result == 0
        && DAT_0018e8c == 200
        && cleared()
        && untouched()
        && resource_calls == 1
        && resource_size == 8000;
    report(ok, '2');

    reset();
    largest = 8079;
    result = replay_buffer_init();
    ok = result == 1
        && DAT_0018e8c == 100
        && DAT_0018eb0 == 100
        && DAT_0018ed0 == (int)pool_image + 4000;
    report(ok, '3');

    reset();
    largest = 8120;
    result = replay_buffer_init();
    ok = result == 1
        && DAT_0018e8c == 101
        && DAT_0018eb0 == 101
        && DAT_0018ed0 == (int)pool_image + 4040;
    report(ok, '4');

    reset();
    largest = 40000;
    result = replay_buffer_init();
    ok = result == 1
        && DAT_0018e8c == 500
        && DAT_0018eb0 == 500
        && DAT_0018ed0 == (int)pool_image + 20000;
    report(ok, '5');

    put('D');
    quit();
    return 0;
}
