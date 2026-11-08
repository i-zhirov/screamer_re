/* The test harness for the rebuilt `lzw_block` module.
 *
 * The harness holds the 0x8490-byte decoder context, the output buffer, and
 * the `lzw_bit_read` stub.  The stub returns a scripted code sequence.  The
 * harness runs nine paths:
 *
 *     0  the end code returns 0 and writes no byte
 *     1  the clear code and a literal set the width, the threshold, the next
 *        entry, the previous code, and the two characters, and write one byte
 *     2  a second literal writes its byte and adds the dictionary entry
 *     3  the next free entry repeats the previous string: the output is A A A
 *     4  a dictionary code walks the prefix chain: the output is A B A B
 *     5  the output limit stops the string in the middle: the state becomes 2,
 *        and the next call drains the rest and adds the entry
 *     6  the next entry reaches the threshold: the width grows to 10
 *     7  the width is 12 at the threshold: the width stays at 12
 *     8  the clear code reaches the output limit: the state becomes 1
 *     D  the exit marker
 */
#pragma off (check_stack)

int lzw_block(char *context);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define CONTEXT_SIZE 0x8490

static char context[CONTEXT_SIZE];
static unsigned char output[0x100];
static char text[8];
static int script[16];
static int script_count;
static int script_index;

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
    *(char **)(context + 0x8488) = (char *)output;
}

static void use_script(int count)
{
    script_count = count;
    script_index = 0;
}

int lzw_bit_read(char *context_pointer)
{
    (void)context_pointer;
    if (script_index >= script_count) {
        return 0x101;
    }
    script_index = script_index + 1;
    return script[script_index - 1];
}

int main(void)
{
    int result;
    int ok;

    reset();
    script[0] = 0x101;
    use_script(1);
    result = lzw_block(context);
    ok = result == 0 && get(0x4070) == 0;
    put(ok ? '0' : 'x');

    reset();
    script[0] = 0x100;
    script[1] = 0x41;
    use_script(2);
    result = lzw_block(context);
    ok = result == 1
        && get(0x4068) == 9
        && get(0x4060) == 0x200
        && get(0x405c) == 0x102
        && get(0x4050) == 0x41
        && get(0x4054) == 0x41
        && context[0x8084] == 0x41
        && context[0x8085] == 0x41
        && get(0x4070) == 1
        && output[0] == 0x41;
    put(ok ? '1' : 'x');

    reset();
    script[0] = 0x100;
    script[1] = 0x41;
    script[2] = 0x42;
    use_script(3);
    lzw_block(context);
    result = lzw_block(context);
    ok = result == 1
        && get(0x4070) == 2
        && output[0] == 0x41
        && output[1] == 0x42
        && *(unsigned short *)(context + 0x102 * 4) == 0x41
        && context[0x102 * 4 + 2] == 0x42
        && get(0x405c) == 0x103
        && get(0x4054) == 0x42;
    put(ok ? '2' : 'x');

    reset();
    script[0] = 0x100;
    script[1] = 0x41;
    script[2] = 0x102;
    use_script(3);
    lzw_block(context);
    result = lzw_block(context);
    ok = result == 1
        && get(0x4070) == 3
        && output[0] == 0x41
        && output[1] == 0x41
        && output[2] == 0x41
        && *(unsigned short *)(context + 0x102 * 4) == 0x41
        && context[0x102 * 4 + 2] == 0x41
        && get(0x405c) == 0x103
        && get(0x4054) == 0x102;
    put(ok ? '3' : 'x');

    reset();
    script[0] = 0x100;
    script[1] = 0x41;
    script[2] = 0x42;
    script[3] = 0x102;
    use_script(4);
    lzw_block(context);
    lzw_block(context);
    result = lzw_block(context);
    ok = result == 1
        && get(0x4070) == 4
        && output[0] == 0x41
        && output[1] == 0x42
        && output[2] == 0x41
        && output[3] == 0x42
        && *(unsigned short *)(context + 0x103 * 4) == 0x42
        && context[0x103 * 4 + 2] == 0x41
        && get(0x405c) == 0x104
        && get(0x4054) == 0x102;
    put(ok ? '4' : 'x');

    reset();
    script[0] = 0x100;
    script[1] = 0x41;
    script[2] = 0x42;
    script[3] = 0x102;
    script[4] = 0x103;
    use_script(5);
    lzw_block(context);
    lzw_block(context);
    lzw_block(context);
    set(0x4048, 5);
    result = lzw_block(context);
    ok = result == 0
        && get(0x404c) == 2
        && get(0x4070) == 5
        && output[4] == 0x42;
    set(0x4048, 0x100);
    result = lzw_block(context);
    ok = ok
        && result == 1
        && get(0x404c) == 0
        && get(0x4070) == 6
        && output[5] == 0x41;
    put(ok ? '5' : 'x');

    reset();
    set(0x405c, 0x200);
    set(0x4060, 0x200);
    set(0x4068, 9);
    set(0x4054, 0x41);
    set(0x4048, 0x100);
    script[0] = 0x42;
    use_script(1);
    result = lzw_block(context);
    ok = result == 1
        && get(0x405c) == 0x201
        && get(0x4068) == 10
        && get(0x4060) == 0x400
        && get(0x4054) == 0x42
        && get(0x4070) == 1
        && output[0] == 0x42;
    put(ok ? '6' : 'x');

    reset();
    set(0x405c, 0x1ff);
    set(0x4060, 0x200);
    set(0x4068, 12);
    set(0x4054, 0x41);
    set(0x4048, 0x100);
    script[0] = 0x42;
    use_script(1);
    result = lzw_block(context);
    ok = result == 1
        && get(0x405c) == 0x200
        && get(0x4068) == 12
        && get(0x4060) == 0x200;
    put(ok ? '7' : 'x');

    reset();
    set(0x4048, 1);
    script[0] = 0x100;
    script[1] = 0x41;
    use_script(2);
    result = lzw_block(context);
    ok = result == 0
        && get(0x404c) == 1
        && get(0x4070) == 1
        && output[0] == 0x41;
    put(ok ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
