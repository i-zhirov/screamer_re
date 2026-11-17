/* The test harness for the rebuilt `lzw_init` module.
 *
 * The harness holds two 0x8490-byte decoder contexts and fills them with the
 * sentinel byte 0x5a.  It runs these paths:
 *
 *     0  a fresh context: the return value is the context, the four width
 *        masks, the zero stack count, the zero bit position, the zero state,
 *        the width 9, the threshold 0x200, and the next entry 0x102
 *     1  the stack count starts non-zero: the clear at +0x4064 and the copy to
 *        the bit position at +0x406c both reach zero
 *     2  a second context: the same fields and the same return value
 *     3  the neighbour cells +0x8070, +0x8084, +0x4058, and +0x4070 keep the
 *        sentinel, so the write set is exact
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

char *lzw_init(char *context);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define CONTEXT_SIZE 0x8490
#define SENTINEL 0x5a5a5a5a

static char context[CONTEXT_SIZE];
static char other[CONTEXT_SIZE];
static char text[8];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int get(char *buffer, int offset)
{
    return *(int *)(buffer + offset);
}

static void set(char *buffer, int offset, int value)
{
    *(int *)(buffer + offset) = value;
}

static void reset(char *buffer)
{
    int index;

    for (index = 0; index < CONTEXT_SIZE; index++) {
        buffer[index] = 0x5a;
    }
}

static int fields_ok(char *buffer)
{
    return get(buffer, 0x8074) == 0x1ff
        && get(buffer, 0x8078) == 0x3ff
        && get(buffer, 0x807c) == 0x7ff
        && get(buffer, 0x8080) == 0xfff
        && get(buffer, 0x4064) == 0
        && get(buffer, 0x406c) == 0
        && get(buffer, 0x404c) == 0
        && get(buffer, 0x4068) == 9
        && get(buffer, 0x4060) == 0x200
        && get(buffer, 0x405c) == 0x102;
}

int main(void)
{
    char *result;

    reset(context);
    result = lzw_init(context);
    put(result == context && fields_ok(context) ? '0' : 'x');

    reset(context);
    set(context, 0x4064, 0x12345678);
    set(context, 0x406c, 0x99);
    lzw_init(context);
    put(get(context, 0x4064) == 0 && get(context, 0x406c) == 0 ? '1' : 'x');

    reset(other);
    result = lzw_init(other);
    put(result == other && fields_ok(other) ? '2' : 'x');

    reset(context);
    lzw_init(context);
    put(get(context, 0x8070) == SENTINEL
        && get(context, 0x8084) == SENTINEL
        && get(context, 0x4058) == SENTINEL
        && get(context, 0x4070) == SENTINEL ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
