/* The test harness for the rebuilt `pcx_decode` module.
 *
 * The harness holds the two argument texts, the PCX member image, the
 * destination buffer, the palette buffer, the output slot, and a stub for
 * every call target.  The stream stubs serve the member image from a buffer and
 * keep the position, so the header read, the RLE reads, and the palette read
 * follow each other.
 *
 * The harness runs five paths:
 *
 *     0  mode 3 decodes a 3x2 image with two RLE runs and one literal byte:
 *        the open, the header read, the pixel writes, the palette read, and
 *        the close, with the output slot kept
 *     1  mode 2 returns the pixel count 6 and closes the stream after the
 *        header read, with no resource and no palette read
 *     2  mode 1 gets the PCX resource of the pixel count, stores the pointer in
 *        the output slot, and decodes the member
 *     3  the open fails, so the function returns -2 with no read, no resource,
 *        and no close
 *     4  mode 1 gets a zero resource, so the function returns -1 with no close
 *     D  the exit marker
 *
 * The input texts and the expected texts are built by character stores,
 * because the `print` helper of this harness writes garbage for a string
 * literal.
 */
#pragma off (check_stack)

int pcx_decode(char *path, char *name, int *out, unsigned char *palette,
               int mode);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_0021574;

static char text[8];
static char path_text[0x10];
static char name_text[0x10];
static char mode_expected[4];
static char resource_expected[4];

static unsigned char file_data[0x1000];
static unsigned char pixels[0x200];
static unsigned char palette[0x300];
static int file_size;
static int position;
static int out_value;

static char open_path[0x10];
static char open_name[0x10];
static char open_category[4];
static int open_result;
static int open_calls;

static void *resource_resource;
static char resource_name[4];
static int resource_size;
static void *resource_result;
static int resource_calls;

static int read_calls;
static int close_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int same(char *left, char *right)
{
    int index;

    for (index = 0; left[index] == right[index]; ++index) {
        if (left[index] == 0) {
            return 1;
        }
    }
    return 0;
}

static void copy(char *destination, char *source)
{
    int index;

    for (index = 0; source[index] != 0; ++index) {
        destination[index] = source[index];
    }
    destination[index] = 0;
}

static void fill_bytes(unsigned char *buffer, int size, int value)
{
    int index;

    for (index = 0; index < size; ++index) {
        buffer[index] = (unsigned char)value;
    }
}

static void set_short(unsigned char *buffer, int offset, int value)
{
    *(short *)(buffer + offset) = (short)value;
}

static void set_texts(void)
{
    path_text[0] = 'G';
    path_text[1] = 'R';
    path_text[2] = 'A';
    path_text[3] = 'F';
    path_text[4] = 0;

    name_text[0] = 'M';
    name_text[1] = 'E';
    name_text[2] = 'N';
    name_text[3] = 'U';
    name_text[4] = '1';
    name_text[5] = '.';
    name_text[6] = 'P';
    name_text[7] = 'C';
    name_text[8] = 'X';
    name_text[9] = 0;

    mode_expected[0] = 'r';
    mode_expected[1] = 'b';
    mode_expected[2] = 0;
    mode_expected[3] = 0;

    resource_expected[0] = 'P';
    resource_expected[1] = 'C';
    resource_expected[2] = 'X';
    resource_expected[3] = 0;
}

static void setup_image(int width, int height, int payload)
{
    int index;
    int base;

    fill_bytes(file_data, 0x1000, 0);
    set_short(file_data, 0x04, 0);
    set_short(file_data, 0x06, 0);
    set_short(file_data, 0x08, width - 1);
    set_short(file_data, 0x0a, height - 1);
    file_size = 0x80 + payload + 0x300;
    base = 0x80 + payload;
    for (index = 0; index < 0x300; ++index) {
        file_data[base + index] = (unsigned char)((index * 5 + 1) & 0xff);
    }
}

static int run_is(int first, int count, int value)
{
    int index;

    for (index = 0; index < count; ++index) {
        if (pixels[first + index] != value) {
            return 0;
        }
    }
    return 1;
}

static int palette_matches(void)
{
    int index;

    for (index = 0; index < 0x300; ++index) {
        if (palette[index] != (unsigned char)((index * 5 + 1) & 0xff)) {
            return 0;
        }
    }
    return 1;
}

int fopen(char *stream, char *path, char *name, char *category)
{
    (void)stream;
    copy(open_path, path);
    copy(open_name, name);
    copy(open_category, category);
    open_calls = open_calls + 1;
    position = 0;
    return open_result;
}

int stream_read(char *stream, void *buffer, int count)
{
    unsigned char *out;
    int index;

    (void)stream;
    read_calls = read_calls + 1;
    out = buffer;
    for (index = 0; index < count && position < file_size; ++index) {
        out[index] = file_data[position++];
    }
    return index;
}

int stream_close(char *stream)
{
    (void)stream;
    close_calls = close_calls + 1;
    return 0;
}

void *resource_get(void *resource, char *name, int size)
{
    resource_resource = resource;
    copy(resource_name, name);
    resource_size = size;
    resource_calls = resource_calls + 1;
    return resource_result;
}

static void reset(void)
{
    position = 0;
    out_value = 0;

    open_path[0] = 0;
    open_name[0] = 0;
    open_category[0] = 0;
    open_result = 0;
    open_calls = 0;

    resource_resource = 0;
    resource_name[0] = 0;
    resource_size = 0;
    resource_result = 0;
    resource_calls = 0;

    read_calls = 0;
    close_calls = 0;

    DAT_0021574 = (void *)0x11110000;

    fill_bytes(pixels, 0x200, 0xaa);
    fill_bytes(palette, 0x300, 0);
}

static void set_payload(void)
{
    file_data[0x80] = 0xc3;
    file_data[0x81] = 5;
    file_data[0x82] = 1;
    file_data[0x83] = 0xc2;
    file_data[0x84] = 7;
}

int main(void)
{
    int result;

    set_texts();

    reset();
    setup_image(3, 2, 5);
    set_payload();
    out_value = (int)pixels;
    result = pcx_decode(path_text, name_text, &out_value, palette, 3);
    put(result == 0
        && open_calls == 1
        && same(open_path, path_text)
        && same(open_name, name_text)
        && same(open_category, mode_expected)
        && resource_calls == 0
        && close_calls == 1
        && out_value == (int)pixels
        && run_is(0, 3, 5) && pixels[3] == 1 && run_is(4, 2, 7)
        && pixels[6] == 0xaa && palette_matches() ? '0' : 'x');

    reset();
    setup_image(3, 2, 5);
    set_payload();
    out_value = 0x1234;
    result = pcx_decode(path_text, name_text, &out_value, palette, 2);
    put(result == 6
        && open_calls == 1
        && read_calls == 1
        && close_calls == 1
        && resource_calls == 0
        && out_value == 0x1234
        && palette[0] == 0 ? '1' : 'x');

    reset();
    setup_image(3, 2, 5);
    set_payload();
    resource_result = pixels;
    out_value = 0;
    result = pcx_decode(path_text, name_text, &out_value, palette, 1);
    put(result == 0
        && resource_calls == 1
        && resource_resource == DAT_0021574
        && same(resource_name, resource_expected)
        && resource_size == 6
        && out_value == (int)pixels
        && close_calls == 1
        && run_is(0, 3, 5) && pixels[3] == 1 && run_is(4, 2, 7)
        && pixels[6] == 0xaa && palette_matches() ? '2' : 'x');

    reset();
    setup_image(3, 2, 5);
    set_payload();
    open_result = 1;
    out_value = 0x1234;
    result = pcx_decode(path_text, name_text, &out_value, palette, 3);
    put(result == -2
        && open_calls == 1
        && read_calls == 0
        && close_calls == 0
        && resource_calls == 0
        && out_value == 0x1234 ? '3' : 'x');

    reset();
    setup_image(3, 2, 5);
    set_payload();
    resource_result = 0;
    out_value = 0;
    result = pcx_decode(path_text, name_text, &out_value, palette, 1);
    put(result == -1
        && resource_calls == 1
        && read_calls == 1
        && close_calls == 0
        && out_value == 0
        && pixels[0] == 0xaa ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
