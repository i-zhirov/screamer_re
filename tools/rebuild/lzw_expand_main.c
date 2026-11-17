/* The test harness for the rebuilt `lzw_expand` module.
 *
 * The harness holds the 0x8490-byte decoder context, the output buffer, the
 * `fread` stub, and the `lzw_block` stub.  The stubs record their arguments
 * and return scripted values.  The harness runs nine paths:
 *
 *     0  the state is zero: the set fields, the one read, and the return
 *     1  the input size is above 0x400: the read count clamps to 0x400
 *     2  the input size is below 0x400: the read count is the input size, the
 *        read returns a smaller count, and the input position takes it
 *     3  the state is two: the reset and the read do not run
 *     4  the block decoder returns 1 three times and then 0: the loop count
 *     5  the block decoder advances the output position: the return value
 *     6  the input size is zero: the read count is zero
 *     7  a second output buffer: the stored buffer pointer
 *     8  the output limit does not steer the read count
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

int lzw_expand(char *context, char *output, int input_size, int count,
               int handle);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define CONTEXT_SIZE 0x8490

static char context[CONTEXT_SIZE];
static char output[0x100];
static char other[0x100];
static char text[8];

static int read_buffer;
static int read_size;
static int read_count;
static int read_stream;
static int read_calls;
static int read_return;

static int block_calls;
static int block_script[16];
static int block_count;
static int block_index;
static int block_increment;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int get(int offset)
{
    return *(int *)(context + offset);
}

static void set(int offset, int value)
{
    *(int *)(context + offset) = value;
}

static void reset(void)
{
    int index;

    for (index = 0; index < CONTEXT_SIZE; index++) {
        context[index] = 0;
    }
    read_buffer = 0;
    read_size = 0;
    read_count = 0;
    read_stream = 0;
    read_calls = 0;
    read_return = 0;
    block_calls = 0;
    block_count = 0;
    block_index = 0;
    block_increment = 0;
}

static void use_script(int count)
{
    block_count = count;
    block_index = 0;
}

int fread(void *buffer, int size, int count, int stream)
{
    read_buffer = (int)buffer;
    read_size = size;
    read_count = count;
    read_stream = stream;
    read_calls = read_calls + 1;
    return read_return;
}

int lzw_block(char *context_pointer)
{
    int result;

    block_calls = block_calls + 1;
    *(int *)(context_pointer + 0x4070) += block_increment;
    if (block_index >= block_count) {
        return 0;
    }
    result = block_script[block_index];
    block_index = block_index + 1;
    return result;
}

int main(void)
{
    int result;
    int ok;

    reset();
    set(0x404c, 0);
    read_return = 0x100;
    block_script[0] = 0;
    use_script(1);
    result = lzw_expand(context, output, 0x100, 0x80, 0x1234);
    ok = get(0x848c) == 0x1234
        && get(0x8488) == (int)output
        && get(0x4044) == 0x100
        && get(0x4048) == 0x80
        && get(0x4070) == 0
        && get(0x4040) == 0x100
        && read_calls == 1
        && read_buffer == (int)(context + 0x8086)
        && read_size == 1
        && read_count == 0x100
        && read_stream == 0x1234
        && result == 0;
    put(ok ? '0' : 'x');

    reset();
    set(0x404c, 0);
    read_return = 0x400;
    block_script[0] = 0;
    use_script(1);
    lzw_expand(context, output, 0x500, 0x80, 0x1234);
    ok = read_count == 0x400 && get(0x4040) == 0x400;
    put(ok ? '1' : 'x');

    reset();
    set(0x404c, 0);
    read_return = 0x123;
    block_script[0] = 0;
    use_script(1);
    lzw_expand(context, output, 0x300, 0x80, 0x1234);
    ok = read_count == 0x300 && get(0x4040) == 0x123;
    put(ok ? '2' : 'x');

    reset();
    set(0x404c, 2);
    set(0x4040, 0x55);
    block_script[0] = 0;
    use_script(1);
    lzw_expand(context, output, 0x100, 0x80, 0x1234);
    ok = read_calls == 0 && get(0x4040) == 0x55 && block_calls == 1;
    put(ok ? '3' : 'x');

    reset();
    set(0x404c, 0);
    block_script[0] = 1;
    block_script[1] = 1;
    block_script[2] = 1;
    block_script[3] = 0;
    use_script(4);
    lzw_expand(context, output, 0x100, 0x80, 0x1234);
    ok = block_calls == 4;
    put(ok ? '4' : 'x');

    reset();
    set(0x404c, 0);
    block_increment = 2;
    block_script[0] = 1;
    block_script[1] = 1;
    block_script[2] = 0;
    use_script(3);
    result = lzw_expand(context, output, 0x100, 0x80, 0x1234);
    ok = block_calls == 3 && get(0x4070) == 6 && result == 6;
    put(ok ? '5' : 'x');

    reset();
    set(0x404c, 0);
    read_return = 0;
    block_script[0] = 0;
    use_script(1);
    lzw_expand(context, output, 0, 0x80, 0x1234);
    ok = read_calls == 1 && read_count == 0 && get(0x4040) == 0;
    put(ok ? '6' : 'x');

    reset();
    set(0x404c, 0);
    block_script[0] = 0;
    use_script(1);
    lzw_expand(context, other, 0x100, 0x80, 0x9999);
    ok = get(0x8488) == (int)other && get(0x848c) == 0x9999;
    put(ok ? '7' : 'x');

    reset();
    set(0x404c, 0);
    read_return = 0x200;
    block_script[0] = 0;
    use_script(1);
    lzw_expand(context, output, 0x200, 0x10, 0x1234);
    ok = read_count == 0x200 && get(0x4048) == 0x10;
    put(ok ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
