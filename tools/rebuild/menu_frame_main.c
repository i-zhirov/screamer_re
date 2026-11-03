/* The test harness for the rebuilt `menu_frame` module.
 *
 * The harness holds the race object with the width at +0x134 and the height at
 * +0x120, the pixel and video buffers, the three strings, the flag word, and
 * every menu global.  The stubs record their arguments and count their calls.
 *
 * The harness runs six paths:
 *
 *     0  the normal path with both swap flags: the load, the two present calls,
 *        the copy, both swaps, the fill, every constant, the two screen
 *        offsets, and the longest string all run
 *     1  the odd width and height: the integer division truncates, only the
 *        first string gives the length, and no swap flag is set
 *     2  the load fails: the cleanup runs and the second string gives the
 *        length
 *     3  the flag 0x10 runs the first swap only and the third string gives the
 *        length
 *     4  the flag 0x20 runs the second swap only
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void menu_frame(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char *DAT_0015ccc;
float DAT_0018e08;
float DAT_001d224;
float DAT_0020824;
float DAT_0020828;
void *DAT_001fad0;
void *DAT_0021528;
unsigned int DAT_001d274;
int DAT_001d234;
int DAT_001fac0;
int DAT_001fac4;
int DAT_00203b8;
int DAT_000007f9;
int DAT_001fab8;
int DAT_001d240;
float DAT_0020378;
float DAT_0020380;
int DAT_001fac8;
char *DAT_001fc1c;
char *DAT_001fc20;
char *DAT_001fc24;
int DAT_001fb64;
int DAT_00203b0;
int DAT_00203a4;
int DAT_001fb58;
int DAT_001d248;
int DAT_00203a8;
int DAT_002039c;
int DAT_00203ac;
int DAT_00203a0;
int DAT_00109a0;
int DAT_0020384;
int DAT_00109a4;
int DAT_0020388;
int DAT_0020394;
int DAT_00109a8;
int DAT_002038c;
int DAT_002037c;
int DAT_00203b4;

static char text[8];
static char context[0x200];
static char pixel_buffer[8];
static char video_buffer[8];
static char string_a[16];
static char string_b[16];
static char string_c[16];

static int pcx_calls;
static char *pcx_file;
static char *pcx_member;
static void *pcx_pixels;
static char *pcx_palette;
static int pcx_result;
static int cleanup_calls;
static int present_calls;
static unsigned int present_scale[2];
static int present_offset[2];
static int memcpy_calls;
static void *memcpy_target;
static void *memcpy_source;
static unsigned int memcpy_count;
static int swap_a_calls;
static int swap_b_calls;
static int fill_calls;
static int init_calls;
static int table_calls;
static int table_index;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static unsigned int bits(float value)
{
    return *(unsigned int *)&value;
}

int pcx_load(char *file, char *member, void *pixels, char *palette)
{
    pcx_file = file;
    pcx_member = member;
    pcx_pixels = pixels;
    pcx_palette = palette;
    pcx_calls = pcx_calls + 1;
    return pcx_result;
}

void sub_15150(void) { cleanup_calls = cleanup_calls + 1; }

void video_present(float scale, int offset)
{
    if (present_calls < 2) {
        present_scale[present_calls] = bits(scale);
        present_offset[present_calls] = offset;
    }
    present_calls = present_calls + 1;
}

void *memcpy(void *target, const void *source, unsigned int count)
{
    memcpy_target = target;
    memcpy_source = (void *)source;
    memcpy_count = count;
    memcpy_calls = memcpy_calls + 1;
    return target;
}

void video_block_swap_a(void) { swap_a_calls = swap_a_calls + 1; }
void video_block_swap_b(void) { swap_b_calls = swap_b_calls + 1; }
void video_fill(void) { fill_calls = fill_calls + 1; }
void sub_2dff4(void) { init_calls = init_calls + 1; }

int table_lookup(int index)
{
    table_index = index;
    table_calls = table_calls + 1;
    return 0;
}

static void set_length(char *text, int length)
{
    int index;

    for (index = 0; index < length; ++index) {
        text[index] = 'a';
    }
    text[length] = 0;
}

static void set_field(int offset, int value)
{
    *(int *)(context + offset) = value;
}

static void reset(void)
{
    DAT_0015ccc = (unsigned char *)context;
    set_field(0x134, 0x140);
    set_field(0x120, 0xc8);
    DAT_001fad0 = pixel_buffer;
    DAT_0021528 = video_buffer;
    DAT_001d274 = 0;
    DAT_001d234 = 0x11111111;
    DAT_001fac0 = 0x11111111;
    DAT_001fac4 = 0x11111111;
    DAT_00203b8 = 0x11111111;
    DAT_0018e08 = 1.0f;
    DAT_000007f9 = 0x11111111;
    DAT_001fab8 = 0x11111111;
    DAT_001d240 = 0x11111111;
    DAT_0020378 = 1.0f;
    DAT_0020380 = 1.0f;
    DAT_001fac8 = 0x11111111;
    set_length(string_a, 3);
    set_length(string_b, 5);
    set_length(string_c, 2);
    DAT_001fc1c = string_a;
    DAT_001fc20 = string_b;
    DAT_001fc24 = string_c;
    DAT_001fb64 = 0x11111111;
    DAT_00203b0 = 0x11111111;
    DAT_00203a4 = 0x11111111;
    DAT_001fb58 = 0x11111111;
    DAT_001d224 = 1.0f;
    DAT_001d248 = 0x11111111;
    DAT_00203a8 = 0x11111111;
    DAT_002039c = 0x11111111;
    DAT_00203ac = 0x11111111;
    DAT_00203a0 = 0x11111111;
    DAT_00109a0 = 0x11111111;
    DAT_0020384 = 0x11111111;
    DAT_00109a4 = 0x11111111;
    DAT_0020388 = 0x11111111;
    DAT_0020394 = 0x11111111;
    DAT_00109a8 = 0x11111111;
    DAT_002038c = 0x11111111;
    DAT_002037c = 0x11111111;
    DAT_00203b4 = 0x11111111;
    pcx_calls = 0;
    pcx_file = 0;
    pcx_member = 0;
    pcx_pixels = 0;
    pcx_palette = 0;
    pcx_result = 0;
    cleanup_calls = 0;
    present_calls = 0;
    present_scale[0] = 0x11111111;
    present_scale[1] = 0x11111111;
    present_offset[0] = 0x11111111;
    present_offset[1] = 0x11111111;
    memcpy_calls = 0;
    memcpy_target = 0;
    memcpy_source = 0;
    memcpy_count = 0;
    swap_a_calls = 0;
    swap_b_calls = 0;
    fill_calls = 0;
    init_calls = 0;
    table_calls = 0;
    table_index = 0x11111111;
}

static int call_site(void)
{
    return pcx_calls == 1
        && pcx_file == (char *)0xfe0
        && pcx_member == (char *)0xfd5
        && pcx_pixels == DAT_001fad0
        && pcx_palette == (char *)0xcf5c
        && present_calls == 2
        && present_scale[0] == 0 && present_offset[0] == 0
        && present_scale[1] == 0 && present_offset[1] == 0
        && memcpy_calls == 1
        && memcpy_target == DAT_0021528
        && memcpy_source == DAT_001fad0
        && memcpy_count == 0x3e800
        && fill_calls == 1
        && init_calls == 1
        && table_calls == 1 && table_index == 0;
}

static int constants(void)
{
    return DAT_001d234 == 0
        && DAT_001fac0 == 0
        && DAT_001fac4 == 0
        && DAT_00203b8 == 0
        && bits(DAT_0018e08) == 0x3da3d70a
        && DAT_000007f9 == 8
        && DAT_001fab8 == 0x35e
        && DAT_001d240 == 1
        && bits(DAT_0020378) == 0x3f19999a
        && bits(DAT_0020380) == 0x3f19999a
        && DAT_001fac8 == 1
        && DAT_001fb64 == -1
        && DAT_00203b0 == 0x1004
        && DAT_00203a4 == 0x1004
        && DAT_001fb58 == 0
        && bits(DAT_001d224) == 0
        && DAT_001d248 == 0
        && DAT_00203a8 == 0
        && DAT_002039c == 0
        && DAT_00203ac == 0
        && DAT_00203a0 == 0
        && DAT_00109a0 == 0
        && DAT_0020384 == 0
        && DAT_00109a4 == 0
        && DAT_0020388 == 0
        && DAT_0020394 == 0
        && DAT_00109a8 == 0
        && DAT_002038c == 0
        && DAT_002037c == 0;
}

int main(void)
{
    reset();
    DAT_001d274 = 0x30;
    menu_frame();
    put(call_site() && constants() && cleanup_calls == 0
        && swap_a_calls == 1 && swap_b_calls == 1
        && bits(DAT_0020824) == 0xc3a00000
        && bits(DAT_0020828) == 0xc42c8000
        && DAT_00203b4 == 5 ? '0' : 'x');

    reset();
    set_field(0x134, 325);
    set_field(0x120, 205);
    set_length(string_a, 8);
    set_length(string_b, 4);
    set_length(string_c, 2);
    menu_frame();
    put(call_site() && constants() && cleanup_calls == 0
        && swap_a_calls == 0 && swap_b_calls == 0
        && bits(DAT_0020824) == 0xc3a08000
        && bits(DAT_0020828) == 0xc42cc000
        && DAT_00203b4 == 8 ? '1' : 'x');

    reset();
    pcx_result = 7;
    set_field(0x134, 324);
    set_field(0x120, 200);
    set_length(string_a, 2);
    set_length(string_b, 6);
    set_length(string_c, 4);
    menu_frame();
    put(call_site() && constants() && cleanup_calls == 1
        && swap_a_calls == 0 && swap_b_calls == 0
        && bits(DAT_0020824) == 0xc3a08000
        && bits(DAT_0020828) == 0xc42c8000
        && DAT_00203b4 == 6 ? '2' : 'x');

    reset();
    DAT_001d274 = 0x10;
    set_length(string_a, 2);
    set_length(string_b, 3);
    set_length(string_c, 7);
    menu_frame();
    put(call_site() && constants() && cleanup_calls == 0
        && swap_a_calls == 1 && swap_b_calls == 0
        && DAT_00203b4 == 7 ? '3' : 'x');

    reset();
    DAT_001d274 = 0x20;
    set_length(string_a, 1);
    set_length(string_b, 2);
    set_length(string_c, 3);
    menu_frame();
    put(call_site() && constants() && cleanup_calls == 0
        && swap_a_calls == 0 && swap_b_calls == 1
        && DAT_00203b4 == 3 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
