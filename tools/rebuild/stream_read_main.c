/* The test harness for the rebuilt `stream_read` module.
 *
 * The harness holds a stream record, the input and output buffers, the raw
 * `fread` stub, the expander stub, and the argument recorders.  The stubs
 * return the configured byte counts.  The harness runs five paths:
 *
 *     0  the compressed path advanced the position and ran the expander with
 *        the buffer at `stream + 0xb0`, the field at `stream + 0x6c`, the
 *        count, and the handle
 *     1  the compressed path clamped the count to the size and set the
 *        position to the size
 *     2  the raw path advanced the position and ran `fread` with the size 1
 *     3  the raw path clamped the result to the size and set the position to
 *        the size
 *     4  the raw path at the end read zero bytes and kept the position
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int stream_read(char *stream, char *buffer, int count);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char record[0xc0];
static char input_buffer[0x20];
static char read_buffer[0x20];

static int fread_result;
static int lzw_result;
static int fread_calls;
static void *fread_buffer;
static int fread_size;
static int fread_count;
static int fread_stream;
static int lzw_calls;
static void *lzw_input;
static void *lzw_output;
static int lzw_input_size;
static int lzw_count;
static int lzw_handle;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int fread(void *buffer, int size, int count, int stream)
{
    fread_buffer = buffer;
    fread_size = size;
    fread_count = count;
    fread_stream = stream;
    fread_calls = fread_calls + 1;
    return fread_result;
}

int lzw_expand(void *input, void *output, int input_size, int count,
               int handle)
{
    lzw_input = input;
    lzw_output = output;
    lzw_input_size = input_size;
    lzw_count = count;
    lzw_handle = handle;
    lzw_calls = lzw_calls + 1;
    return lzw_result;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0xc0; ++index) {
        record[index] = 0;
    }
    *(int *)record = 3;
    *(int *)(record + 0x8) = 100;
    *(void **)(record + 0xb0) = input_buffer;
    fread_calls = 0;
    lzw_calls = 0;
}

int main(void)
{
    int result;

    reset();
    *(int *)(record + 0x10) = 1;
    *(int *)(record + 0xc) = 5;
    *(int *)(record + 0x6c) = 0x1234;
    *(int *)record = 7;
    lzw_result = 7;
    result = stream_read((char *)record, read_buffer, 10);
    put(result == 7 && *(int *)(record + 0xc) == 15 && lzw_calls == 1
        && lzw_input == input_buffer && lzw_output == read_buffer
        && lzw_input_size == 0x1234 && lzw_count == 10 && lzw_handle == 7
        && fread_calls == 0 ? '0' : 'x');

    reset();
    *(int *)(record + 0x10) = 1;
    *(int *)(record + 0xc) = 95;
    lzw_result = 5;
    result = stream_read((char *)record, read_buffer, 10);
    put(result == 5 && *(int *)(record + 0xc) == 100 && lzw_calls == 1
        && lzw_count == 5 ? '1' : 'x');

    reset();
    *(int *)(record + 0x10) = 0;
    *(int *)(record + 0xc) = 20;
    *(int *)record = 4;
    fread_result = 8;
    result = stream_read((char *)record, read_buffer, 10);
    put(result == 8 && *(int *)(record + 0xc) == 28 && fread_calls == 1
        && fread_buffer == read_buffer && fread_size == 1
        && fread_count == 10 && fread_stream == 4 && lzw_calls == 0
        ? '2' : 'x');

    reset();
    *(int *)(record + 0x10) = 0;
    *(int *)(record + 0xc) = 80;
    fread_result = 50;
    result = stream_read((char *)record, read_buffer, 50);
    put(result == 20 && *(int *)(record + 0xc) == 100 && fread_calls == 1
        && fread_count == 50 ? '3' : 'x');

    reset();
    *(int *)(record + 0x10) = 0;
    *(int *)(record + 0xc) = 100;
    fread_result = 0;
    result = stream_read((char *)record, read_buffer, 10);
    put(result == 0 && *(int *)(record + 0xc) == 100 && fread_calls == 1
        ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
