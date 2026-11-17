/* The test harness for the rebuilt `member_open` module.
 *
 * The harness holds the stream record, the simulated directory, and the stubs
 * of the file layer and the decoder layer.  The stubs record their arguments,
 * `fread` returns the 4-byte directory size and then the encoded directory
 * records in order, and `strchr`, `strcpy`, and `stricmp` use their real
 * behaviour.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero, so every
 * text lives in a data buffer that the harness fills at run time.
 *
 * The harness runs seven paths:
 *
 *     0  a plain member in the binary category: the bounds and the clear flags,
 *        no decoder context, and the cleared buffer pointer
 *     1  a compressed member with the line flag: the decoder context, the
 *        output buffer, and the expand call arguments
 *     2  a missing member: the name copy, the stream close, the state 2, and
 *        the result -1
 *     3  a text member in the text category: the close and the reopen with the
 *        path and the category
 *     4  the reopen fails: the result -1 with the cleared handle
 *     5  the decoder allocation fails: the null context and the null expand
 *        context
 *     6  the output allocation fails: the result 5 and no expand call
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

int member_open(char *stream, char *name);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STREAM_SIZE 0x200
#define RECORD_SIZE 0x20
#define MAX_RECORDS 4
#define MAX_CALLS 8

static char stream[STREAM_SIZE];
static unsigned char directory[MAX_RECORDS][RECORD_SIZE];
static int directory_size;
static int record_count;
static int record_index;

static char text[8];

static char name_member[0x10];
static char name_packed[0x10];
static char name_other[0x10];
static char name_missing[0x10];
static char name_text[0x10];
static char name_empty[0x10];
static char category_rb[4];
static char category_rt[4];
static char path_text[0x20];

static int seek_calls;
static void *seek_stream[MAX_CALLS];
static long seek_offset[MAX_CALLS];
static int seek_whence[MAX_CALLS];
static int read_calls;
static void *read_pointer[MAX_CALLS];
static unsigned int read_size[MAX_CALLS];
static unsigned int read_count[MAX_CALLS];
static int close_calls;
static void *close_handle;
static int open_calls;
static char open_path[0x40];
static char open_mode[0x40];
static int alloc_calls;
static unsigned int alloc_size;
static int init_calls;
static void *init_context;
static int malloc_calls;
static unsigned int malloc_size;
static int expand_calls;
static void *expand_context;
static void *expand_output;
static int expand_input;
static int expand_output_limit;
static void *expand_handle;

static char context_buffer[0x10];
static char decode_buffer[0x10];
static void *alloc_result;
static void *malloc_result;
static void *fopen_result;

static void set_text(char *destination, char a, char b, char c, char d,
    char e, char f, char g, char h)
{
    destination[0] = a;
    destination[1] = b;
    destination[2] = c;
    destination[3] = d;
    destination[4] = e;
    destination[5] = f;
    destination[6] = g;
    destination[7] = h;
    destination[8] = 0;
}

static void init_text(void)
{
    set_text(name_member, 'M', 'E', 'M', 'B', 'E', 'R', 0, 0);
    set_text(name_packed, 'P', 'A', 'C', 'K', 'E', 'D', 0, 0);
    set_text(name_other, 'O', 'T', 'H', 'E', 'R', 0, 0, 0);
    set_text(name_missing, 'M', 'I', 'S', 'S', 'I', 'N', 'G', 0);
    set_text(name_text, 'T', 'E', 'X', 'T', 0, 0, 0, 0);
    set_text(name_empty, 0, 0, 0, 0, 0, 0, 0, 0);
    set_text(category_rb, 'r', 'b', 0, 0, 0, 0, 0, 0);
    set_text(category_rt, 'r', 't', 0, 0, 0, 0, 0, 0);
    set_text(path_text, 'D', 'A', 'T', 'A', '\\', 'M', 'U', 'S');
    set_text(path_text + 8, 'I', 'C', '.', 'D', 'A', 'T', 0, 0);
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

static int same(const char *left, const char *right)
{
    while (*left != 0 && *left == *right) {
        left++;
        right++;
    }
    return *left == *right;
}

static void copy(char *destination, const char *source)
{
    while (*source != 0) {
        *destination++ = *source++;
    }
    *destination = 0;
}

int fseek(void *handle, long offset, int whence)
{
    int which;

    which = seek_calls;
    if (which < MAX_CALLS) {
        seek_stream[which] = handle;
        seek_offset[which] = offset;
        seek_whence[which] = whence;
    }
    seek_calls = seek_calls + 1;
    return 0;
}

unsigned int fread(void *pointer, unsigned int size, unsigned int count,
    void *handle)
{
    int which;
    int position;

    which = read_calls;
    if (which < MAX_CALLS) {
        read_pointer[which] = pointer;
        read_size[which] = size;
        read_count[which] = count;
    }
    read_calls = read_calls + 1;

    if (size == 4 && count == 1) {
        *(int *)pointer = directory_size;
        return 1;
    }
    if (size == RECORD_SIZE && count == 1) {
        if (record_index < record_count) {
            for (position = 0; position < RECORD_SIZE; position++) {
                ((unsigned char *)pointer)[position] =
                    directory[record_index][position];
            }
            record_index = record_index + 1;
            return 1;
        }
        return 0;
    }
    return 0;
}

int fclose(void *handle)
{
    close_handle = handle;
    close_calls = close_calls + 1;
    return 0;
}

void *fopen(const char *path, const char *mode)
{
    copy(open_path, path);
    copy(open_mode, mode);
    open_calls = open_calls + 1;
    return fopen_result;
}

char *strcpy(char *destination, const char *source)
{
    char *result;

    result = destination;
    while (*source != 0) {
        *destination++ = *source++;
    }
    *destination = 0;
    return result;
}

int stricmp(const char *left, const char *right)
{
    unsigned char a;
    unsigned char b;

    for (;;) {
        a = (unsigned char)*left;
        b = (unsigned char)*right;
        if (a >= 'A' && a <= 'Z') {
            a = a + ('a' - 'A');
        }
        if (b >= 'A' && b <= 'Z') {
            b = b + ('a' - 'A');
        }
        if (a != b) {
            return (int)a - (int)b;
        }
        if (a == 0) {
            return 0;
        }
        left++;
        right++;
    }
}

char *strchr(const char *text_value, int character)
{
    char wanted;

    wanted = (char)character;
    for (;;) {
        if (*text_value == wanted) {
            return (char *)text_value;
        }
        if (*text_value == 0) {
            return 0;
        }
        text_value++;
    }
}

void *memory_alloc(unsigned int size)
{
    alloc_size = size;
    alloc_calls = alloc_calls + 1;
    return alloc_result;
}

void *malloc(unsigned int size)
{
    malloc_size = size;
    malloc_calls = malloc_calls + 1;
    return malloc_result;
}

void *lzw_init(void *context)
{
    init_context = context;
    init_calls = init_calls + 1;
    return context;
}

void lzw_expand(void *context, void *output, int input_limit,
    int output_limit, void *handle)
{
    expand_context = context;
    expand_output = output;
    expand_input = input_limit;
    expand_output_limit = output_limit;
    expand_handle = handle;
    expand_calls = expand_calls + 1;
}

static void encode_record(int slot, const char *name, int offset, int size,
    int end, int flags)
{
    int position;

    for (position = 0; position < RECORD_SIZE; position++) {
        directory[slot][position] = 0x3a;
    }
    for (position = 0; name[position] != 0 && position < 0x0f; position++) {
        directory[slot][position] = (unsigned char)(name[position] ^ 0x3a);
    }
    directory[slot][position] = 0x3a;
    *(int *)(directory[slot] + 0x10) = offset;
    *(int *)(directory[slot] + 0x14) = size;
    *(int *)(directory[slot] + 0x18) = end;
    *(int *)(directory[slot] + 0x1c) = flags;
}

static void reset(void)
{
    int position;
    int slot;

    for (position = 0; position < STREAM_SIZE; position++) {
        stream[position] = 0;
    }
    for (slot = 0; slot < MAX_RECORDS; slot++) {
        for (position = 0; position < RECORD_SIZE; position++) {
            directory[slot][position] = 0x3a;
        }
    }
    directory_size = 0;
    record_count = 0;
    record_index = 0;

    *(void **)stream = (void *)0x1111;
    *(int *)(stream + 0x04) = 0x44444444;
    *(int *)(stream + 0x08) = 0x88888888;
    *(int *)(stream + 0x0c) = 0xcccccccc;
    *(int *)(stream + 0x10) = 0x10101010;
    *(int *)(stream + 0x14) = 0x14141414;
    *(int *)(stream + 0x18) = 0x18181818;
    *(int *)(stream + 0x1c) = 0x1c1c1c1c;
    *(int *)(stream + 0xac) = 0xacacacac;
    *(void **)(stream + 0xb0) = (void *)0xb0b0b0b0;
    copy(stream + 0x20, category_rb);
    copy(stream + 0x24, path_text);

    seek_calls = 0;
    read_calls = 0;
    close_calls = 0;
    close_handle = 0;
    open_calls = 0;
    open_path[0] = 0;
    open_mode[0] = 0;
    alloc_calls = 0;
    alloc_size = 0;
    init_calls = 0;
    init_context = 0;
    malloc_calls = 0;
    malloc_size = 0;
    expand_calls = 0;
    expand_context = 0;
    expand_output = 0;
    expand_input = 0;
    expand_output_limit = 0;
    expand_handle = 0;

    alloc_result = context_buffer;
    malloc_result = decode_buffer;
    fopen_result = (void *)0x2222;
}

int main(void)
{
    int result;

    init_text();

    reset();
    encode_record(0, name_member, 0x100, 0x40, 0x140, 0);
    record_count = 1;
    directory_size = RECORD_SIZE;
    result = member_open(stream, name_member);
    put(result == 0
        && seek_calls == 3
        && seek_offset[0] == -4L && seek_whence[0] == 2
        && seek_offset[1] == -(long)(RECORD_SIZE + 4) && seek_whence[1] == 2
        && seek_offset[2] == 0x100L && seek_whence[2] == 0
        && read_calls == 2
        && read_size[0] == 4 && read_size[1] == RECORD_SIZE
        && *(int *)(stream + 0x04) == 0x100
        && *(int *)(stream + 0x08) == 0x240
        && *(int *)(stream + 0x0c) == 0x100
        && *(int *)(stream + 0x10) == 0
        && *(int *)(stream + 0x14) == 0
        && *(int *)(stream + 0x1c) == 0
        && *(void **)(stream + 0xb0) == (void *)0xb0b0b0b0
        && close_calls == 0 && open_calls == 0
        && alloc_calls == 0 && malloc_calls == 0 && expand_calls == 0
        && same(stream + 0x58, name_member)
        ? '0' : 'x');

    reset();
    encode_record(0, name_packed, 0x200, 0x80, 0x280, 3);
    record_count = 1;
    directory_size = RECORD_SIZE;
    copy(stream + 0x20, category_rt);
    result = member_open(stream, name_packed);
    put(result == 0
        && seek_calls == 3
        && seek_offset[2] == 0x200L && seek_whence[2] == 0
        && *(int *)(stream + 0x10) == 2
        && *(int *)(stream + 0x14) == 1
        && *(int *)(stream + 0x08) == 0x480
        && *(int *)(stream + 0x04) == 0x200
        && *(int *)(stream + 0x0c) == 0x200
        && close_calls == 0 && open_calls == 0
        && alloc_calls == 1 && alloc_size == 0x8490
        && init_calls == 1 && init_context == alloc_result
        && *(void **)(stream + 0xb0) == alloc_result
        && malloc_calls == 1 && malloc_size == 0x281
        && *(void **)(stream + 0x1c) == malloc_result
        && expand_calls == 1
        && expand_context == alloc_result
        && expand_output == malloc_result
        && expand_input == 0x80
        && expand_output_limit == 0x281
        && expand_handle == (void *)0x1111
        && *(int *)(stream + 0x18) == 0
        ? '1' : 'x');

    reset();
    encode_record(0, name_other, 0x40, 0x10, 0x50, 0);
    encode_record(1, name_empty, 0, 0, 0, 0);
    record_count = 2;
    directory_size = 2 * RECORD_SIZE;
    result = member_open(stream, name_missing);
    put(result == -1
        && read_calls == 3
        && close_calls == 1 && close_handle == (void *)0x1111
        && *(int *)(stream + 0xac) == 2
        && same(stream + 0x78, name_missing)
        && open_calls == 0 && alloc_calls == 0 && malloc_calls == 0
        && expand_calls == 0
        ? '2' : 'x');

    reset();
    encode_record(0, name_text, 0x300, 0x40, 0x340, 0);
    record_count = 1;
    directory_size = RECORD_SIZE;
    copy(stream + 0x20, category_rt);
    result = member_open(stream, name_text);
    put(result == 0
        && close_calls == 1 && close_handle == (void *)0x1111
        && open_calls == 1
        && same(open_path, path_text)
        && same(open_mode, category_rt)
        && *(void **)stream == (void *)0x2222
        && *(int *)(stream + 0x1c) == 0
        && alloc_calls == 0 && malloc_calls == 0 && expand_calls == 0
        ? '3' : 'x');

    reset();
    encode_record(0, name_text, 0x300, 0x40, 0x340, 0);
    record_count = 1;
    directory_size = RECORD_SIZE;
    copy(stream + 0x20, category_rt);
    fopen_result = 0;
    result = member_open(stream, name_text);
    put(result == -1
        && close_calls == 1 && open_calls == 1
        && *(void **)stream == 0
        && alloc_calls == 0 && malloc_calls == 0 && expand_calls == 0
        ? '4' : 'x');

    reset();
    encode_record(0, name_packed, 0x200, 0x80, 0x280, 3);
    record_count = 1;
    directory_size = RECORD_SIZE;
    copy(stream + 0x20, category_rt);
    alloc_result = 0;
    result = member_open(stream, name_packed);
    put(result == 0
        && alloc_calls == 1
        && init_calls == 0
        && *(void **)(stream + 0xb0) == 0
        && malloc_calls == 1
        && expand_calls == 1 && expand_context == 0
        && *(int *)(stream + 0x18) == 0
        ? '5' : 'x');

    reset();
    encode_record(0, name_packed, 0x200, 0x80, 0x280, 3);
    record_count = 1;
    directory_size = RECORD_SIZE;
    copy(stream + 0x20, category_rt);
    malloc_result = 0;
    result = member_open(stream, name_packed);
    put(result == 5
        && alloc_calls == 1 && init_calls == 1
        && *(void **)(stream + 0x1c) == 0
        && expand_calls == 0
        ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
