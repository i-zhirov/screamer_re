/* The test harness for the rebuilt `lzw_bit_read` module.
 *
 * The harness holds the large reader context, the `fread` stub, and the
 * argument recorders.  The stub records its arguments, fills the destination,
 * and returns the configured byte count.  The mask table and the buffer sit at
 * the real offsets, so the module sees the same layout as the original.
 *
 * The harness runs six paths:
 *
 *     0  no refill: the 9-bit code at bit 0
 *     1  no refill: the 9-bit code at bit 3
 *     2  no refill: the 12-bit code at bit 0
 *     3  refill: three leftover bytes, the free space limits the read, and
 *        the bit position keeps only the sub-byte offset
 *     4  refill: four leftover bytes, the stream limit limits the read, and
 *        the sub-byte offset 5 shifts the code
 *     5  no refill: the 10-bit code at bit 0
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

int lzw_bit_read(char *context);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char context[0x8490];

static int fread_result;
static int fread_fill;
static int fread_calls;
static void *fread_buffer;
static int fread_size;
static int fread_count;
static int fread_stream;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int fread(void *buffer, int size, int count, int stream)
{
    int index;
    char *bytes;

    fread_buffer = buffer;
    fread_size = size;
    fread_count = count;
    fread_stream = stream;
    fread_calls = fread_calls + 1;
    bytes = (char *)buffer;
    for (index = 0; index < count; ++index) {
        bytes[index] = (char)fread_fill;
    }
    return fread_result;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x8490; ++index) {
        context[index] = 0;
    }
    fread_calls = 0;
    fread_result = 0;
    fread_fill = 0;
}

static void set_mask(int shift, unsigned int mask)
{
    *(unsigned int *)(context + 0x8050 + (shift << 2)) = mask;
}

static void set_byte(int offset, int value)
{
    context[0x8086 + offset] = (char)value;
}

int main(void)
{
    int result;

    reset();
    set_mask(9, 0x1ff);
    *(int *)(context + 0x4068) = 9;
    set_byte(0, 0xff);
    set_byte(1, 0x01);
    result = lzw_bit_read(context);
    put(result == 511 && *(int *)(context + 0x406c) == 9
        && fread_calls == 0 ? '0' : 'x');

    reset();
    set_mask(9, 0x1ff);
    *(int *)(context + 0x4068) = 9;
    *(int *)(context + 0x406c) = 3;
    set_byte(0, 0x00);
    set_byte(1, 0x02);
    set_byte(2, 0x00);
    result = lzw_bit_read(context);
    put(result == 64 && *(int *)(context + 0x406c) == 12
        && fread_calls == 0 ? '1' : 'x');

    reset();
    set_mask(12, 0xfff);
    *(int *)(context + 0x4068) = 12;
    set_byte(0, 0xab);
    set_byte(1, 0xcd);
    set_byte(2, 0x0f);
    result = lzw_bit_read(context);
    put(result == 3499 && *(int *)(context + 0x406c) == 12
        && fread_calls == 0 ? '2' : 'x');

    reset();
    set_mask(9, 0x1ff);
    *(int *)(context + 0x4068) = 9;
    *(int *)(context + 0x406c) = 0x3fd * 8;
    *(int *)(context + 0x4040) = 100;
    *(int *)(context + 0x4044) = 200;
    *(int *)(context + 0x848c) = 0x55;
    set_byte(0x3fd, 0x11);
    set_byte(0x3fe, 0x02);
    set_byte(0x3ff, 0x00);
    fread_result = 100;
    fread_fill = 0xaa;
    result = lzw_bit_read(context);
    put(result == 17 && *(int *)(context + 0x406c) == 9
        && fread_calls == 1 && fread_buffer == context + 0x8086 + 3
        && fread_size == 1 && fread_count == 100 && fread_stream == 0x55
        && *(int *)(context + 0x4040) == 200
        && context[0x8086] == 0x11 && context[0x8087] == 0x02
        && context[0x8088] == 0x00 ? '3' : 'x');

    reset();
    set_mask(9, 0x1ff);
    *(int *)(context + 0x4068) = 9;
    *(int *)(context + 0x406c) = 0x3fc * 8 + 5;
    *(int *)(context + 0x4040) = 200;
    *(int *)(context + 0x4044) = 210;
    *(int *)(context + 0x848c) = 0x66;
    set_byte(0x3fc, 0x55);
    set_byte(0x3fd, 0x66);
    set_byte(0x3fe, 0x00);
    set_byte(0x3ff, 0x00);
    fread_result = 10;
    fread_fill = 0xbb;
    result = lzw_bit_read(context);
    put(result == 306 && *(int *)(context + 0x406c) == 14
        && fread_calls == 1 && fread_buffer == context + 0x8086 + 4
        && fread_count == 10 && *(int *)(context + 0x4040) == 210
        && context[0x8086] == 0x55 && context[0x8087] == 0x66
        && context[0x8088] == 0x00 && context[0x8089] == 0x00
        ? '4' : 'x');

    reset();
    set_mask(10, 0x3ff);
    *(int *)(context + 0x4068) = 10;
    set_byte(0, 0xff);
    set_byte(1, 0xff);
    result = lzw_bit_read(context);
    put(result == 1023 && *(int *)(context + 0x406c) == 10
        && fread_calls == 0 ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
