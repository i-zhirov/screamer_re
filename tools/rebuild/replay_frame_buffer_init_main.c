/* The test harness for the rebuilt `replay_frame_buffer_init` module.
 *
 * The harness holds the replay buffer pointer, the frame count, the context
 * pointer, the `REPLAY` name cell, and the two callee stubs.  The
 * `memory_block_largest` stub returns a scripted value, and the `resource_get`
 * stub returns a scripted pointer and records its arguments.
 *
 * The harness runs five paths:
 *
 *     0  the count 199 is below 0xc8: the function sets the count, leaves the
 *        buffer pointer, and calls no resource
 *     1  the count 200 loads the resource: the function returns 1 and stores
 *        the pointer
 *     2  the count 200 with a failed load: the function returns 0 and stores
 *        the zero pointer
 *     3  the count 201 loads the resource: the count confirms the truncation
 *     4  the count 2000 loads the resource: the count confirms the scale
 *     D  the exit marker
 */
#pragma off (check_stack)

int replay_frame_buffer_init(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_000062ac;
char DAT_00100dac[16];
int DAT_0018e5c;
int DAT_0018e68;

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
    DAT_00100dac[0] = 'R';
    DAT_00100dac[1] = 'E';
    DAT_00100dac[2] = 'P';
    DAT_00100dac[3] = 'L';
    DAT_00100dac[4] = 'A';
    DAT_00100dac[5] = 'Y';
    DAT_00100dac[6] = 0;
    DAT_0018e5c = 0x1111;
    DAT_0018e68 = 0x2222;
    largest = 0;
    resource_pointer = pool_image;
    resource_calls = 0;
    resource_context = 0;
    resource_name = 0;
    resource_size = 0;
}

static int name_is_replay(void)
{
    char *name = resource_name;

    return name != 0
        && name[0] == 'R'
        && name[1] == 'E'
        && name[2] == 'P'
        && name[3] == 'L'
        && name[4] == 'A'
        && name[5] == 'Y'
        && name[6] == 0;
}

int main(void)
{
    int result;
    int ok;

    reset();
    largest = 3980;
    result = replay_frame_buffer_init();
    ok = result == 0
        && DAT_0018e68 == 199
        && DAT_0018e5c == 0x1111
        && resource_calls == 0;
    report(ok, '0');

    reset();
    largest = 4000;
    result = replay_frame_buffer_init();
    ok = result == 1
        && DAT_0018e68 == 200
        && DAT_0018e5c == (int)pool_image
        && resource_calls == 1
        && resource_context == pool_image
        && resource_size == 4000
        && name_is_replay();
    report(ok, '1');

    reset();
    largest = 4000;
    resource_pointer = 0;
    result = replay_frame_buffer_init();
    ok = result == 0
        && DAT_0018e68 == 200
        && DAT_0018e5c == 0
        && resource_calls == 1
        && resource_size == 4000
        && name_is_replay();
    report(ok, '2');

    reset();
    largest = 4020;
    result = replay_frame_buffer_init();
    ok = result == 1
        && DAT_0018e68 == 201
        && DAT_0018e5c == (int)pool_image;
    report(ok, '3');

    reset();
    largest = 40000;
    result = replay_frame_buffer_init();
    ok = result == 1
        && DAT_0018e68 == 2000
        && DAT_0018e5c == (int)pool_image;
    report(ok, '4');

    put('D');
    quit();
    return 0;
}
