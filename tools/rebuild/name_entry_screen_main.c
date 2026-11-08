/* The test harness for the rebuilt `name_entry_screen` module.
 *
 * The harness holds the player name, the menu buffers, the palette, the shared
 * heading cells, the 0x50 map object records at `DAT_001db78`, and the seven
 * float constants.  The stubs record the call counts and the arguments.
 *
 * The harness runs five paths:
 *
 *     0  the letter C: the index 2 branch places the marker at -2091 and 3984,
 *        the heading reaches 157, and the 26 records hold the A to Z letters
 *        and the ring positions
 *     1  the letter Z: the index 25 branch places the marker at 1076 and 4369
 *        and the heading reaches 1969
 *     2  the letter A: the index 0 branch clears the marker and stores the
 *        default 0x1194 at the two marker cells
 *     3  the letter B with a failed load: the cleanup runs once and the rest
 *        of the setup still runs
 *     4  the record bound: the first 26 records change and record 26 stays
 *        clear, because the screen builds only the A to Z letters
 *     D  the exit marker
 */
#pragma off (check_stack)

void name_entry_screen(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0011d240;
float DAT_00120378;
float DAT_00120380;
int DAT_0011d244;
int DAT_0011d23c;
int DAT_0011fac8;
unsigned char DAT_001203cc[0x40];
void *DAT_0011fad0;
void *DAT_00121528;
unsigned char DAT_0010cf5c[0x300];

float DAT_00120388;
float DAT_00120394;
float DAT_00120384;
float DAT_0012038c;
float DAT_0012037c;
int DAT_0012039c;
int DAT_001203a0;
int DAT_001203a4;
int DAT_001203a8;
int DAT_001203ac;
int DAT_001203b0;
int DAT_00109a0;
int DAT_00109a8;

float DAT_00118e08;
int DAT_000007f9;
char DAT_001db78[0x50 * 0x64];

float DAT_0010116c;
float DAT_00101170;
float DAT_00101174;
float DAT_00101178;
float DAT_0010117c;
float DAT_00101180;
double DAT_00101184;

static char text[8];
static char pixel_buffer[0x3e800];
static char video_buffer[0x3e800];

static int pcx_calls;
static char *pcx_file;
static char *pcx_member;
static void *pcx_pixels;
static char *pcx_palette;
static int pcx_result;
static int cleanup_calls;
static int present_calls;
static unsigned int present_scale[4];
static int present_offset[4];
static int memcpy_calls;
static void *memcpy_target;
static void *memcpy_source;
static unsigned int memcpy_count;
static int fill_calls;
static int init_calls;
static int heading_calls;

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

static int near_float(float value, float expected, float tolerance)
{
    float difference;

    difference = value - expected;
    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference <= tolerance;
}

int pcx_load(char *file, char *member, void *pixels, char *palette)
{
    pcx_calls = pcx_calls + 1;
    pcx_file = file;
    pcx_member = member;
    pcx_pixels = pixels;
    pcx_palette = palette;
    return pcx_result;
}

void sub_15150(void) { cleanup_calls = cleanup_calls + 1; }

void video_present(float scale, int offset)
{
    if (present_calls < 4) {
        present_scale[present_calls] = bits(scale);
        present_offset[present_calls] = offset;
    }
    present_calls = present_calls + 1;
}

void *memcpy(void *target, const void *source, unsigned int count)
{
    memcpy_calls = memcpy_calls + 1;
    memcpy_target = target;
    memcpy_source = (void *)source;
    memcpy_count = count;
    return target;
}

void video_fill(void) { fill_calls = fill_calls + 1; }

void sub_2dff4(void) { init_calls = init_calls + 1; }

void race_heading_update(void) { heading_calls = heading_calls + 1; }

static void reset(char letter)
{
    int i;

    for (i = 0; i < (int)sizeof(DAT_001db78); ++i) {
        DAT_001db78[i] = 0;
    }
    for (i = 0; i < 0x40; ++i) {
        DAT_001203cc[i] = 0;
    }
    DAT_001203cc[0] = letter;
    DAT_0011d240 = 0x11111111;
    DAT_00120378 = 1.0f;
    DAT_00120380 = 1.0f;
    DAT_0011d244 = 0x11111111;
    DAT_0011d23c = 0x11111111;
    DAT_0011fac8 = 0x11111111;
    DAT_0011fad0 = pixel_buffer;
    DAT_00121528 = video_buffer;
    DAT_00120388 = 1.0f;
    DAT_00120394 = 1.0f;
    DAT_00120384 = 1.0f;
    DAT_0012038c = 1.0f;
    DAT_0012037c = 1.0f;
    DAT_0012039c = 0x11111111;
    DAT_001203a0 = 0x11111111;
    DAT_001203a4 = 0x11111111;
    DAT_001203a8 = 0x11111111;
    DAT_001203ac = 0x11111111;
    DAT_001203b0 = 0x11111111;
    DAT_00109a0 = 0x11111111;
    DAT_00109a8 = 0x11111111;
    DAT_00118e08 = 1.0f;
    DAT_000007f9 = 0x11111111;
    DAT_0010116c = -0.2416609823703766f;
    DAT_00101170 = 4500.0f;
    DAT_00101174 = 0.2416609823703766f;
    DAT_00101178 = 1024.0f;
    DAT_0010117c = 3.1415927410125732f;
    DAT_00101180 = 16.0f;
    DAT_00101184 = 0.04;
    pcx_calls = 0;
    pcx_file = 0;
    pcx_member = 0;
    pcx_pixels = 0;
    pcx_palette = 0;
    pcx_result = 0;
    cleanup_calls = 0;
    present_calls = 0;
    present_scale[0] = 0;
    present_offset[0] = 0;
    present_scale[1] = 0;
    present_offset[1] = 0;
    memcpy_calls = 0;
    memcpy_target = 0;
    memcpy_source = 0;
    memcpy_count = 0;
    fill_calls = 0;
    init_calls = 0;
    heading_calls = 0;
}

static int record_int(int index, int offset)
{
    return *(int *)(DAT_001db78 + index * 0x64 + offset);
}

static float record_float(int index, int offset)
{
    return *(float *)(DAT_001db78 + index * 0x64 + offset);
}

static int call_site(void)
{
    return pcx_calls == 1
        && pcx_file == (char *)0x115f
        && pcx_member == (char *)0x1154
        && pcx_pixels == DAT_0011fad0
        && pcx_palette == (char *)0xcf5c
        && present_calls == 2
        && present_scale[0] == 0 && present_offset[0] == 0
        && present_scale[1] == 0 && present_offset[1] == 0
        && memcpy_calls == 1
        && memcpy_target == DAT_00121528
        && memcpy_source == DAT_0011fad0
        && memcpy_count == 0x3e800
        && fill_calls == 1
        && init_calls == 1
        && heading_calls == 1;
}

static int records(void)
{
    int i;

    for (i = 0; i < 26; ++i) {
        if (record_int(i, 4) != i + 0x41) {
            return 0;
        }
        if (record_int(i, 8) != i + 0x21) {
            return 0;
        }
        if (record_int(i, 0x10) != 0x400cbe4d) {
            return 0;
        }
        if (record_int(i, 0xc) != 1) {
            return 0;
        }
        if (record_int(i, 0x18) != 0) {
            return 0;
        }
        if (record_int(i, 0x38) != 160) {
            return 0;
        }
        if (record_int(i, 0x3c) != 3500) {
            return 0;
        }
        if (record_int(i, 0x48) != 3500) {
            return 0;
        }
        if (record_int(i, 0x58) != 0) {
            return 0;
        }
        if (bits(record_float(i, 0x5c)) != 0x3f800000) {
            return 0;
        }
        if (!near_float(record_float(i, 0x14),
                (float)i * DAT_00101174, 0.00001f)) {
            return 0;
        }
        if (record_int(i, 0x44) != 2560 + (i & 1) * 500) {
            return 0;
        }
    }
    if (record_int(0, 0x34) != 0 || record_int(0, 0x40) != 0) {
        return 0;
    }
    if (!near_float(record_float(0, 0x60), 0.0f, 0.00001f)) {
        return 0;
    }
    if (record_int(1, 0x34) != -837 || record_int(1, 0x40) != -837) {
        return 0;
    }
    if (!near_float(record_float(1, 0x60), 0.04f, 0.00001f)) {
        return 0;
    }
    if (record_int(2, 0x34) != -1626 || record_int(2, 0x40) != -1626) {
        return 0;
    }
    if (!near_float(record_float(2, 0x60), 0.08f, 0.00001f)) {
        return 0;
    }
    if (record_int(25, 0x34) != 837 || record_int(25, 0x40) != 837) {
        return 0;
    }
    if (!near_float(record_float(25, 0x60), 1.0f, 0.00001f)) {
        return 0;
    }
    return 1;
}

int main(void)
{
    reset('C');
    name_entry_screen();
    put(DAT_0011d240 == 0
        && bits(DAT_00120378) == 0x3fc00000
        && bits(DAT_00120380) == 0x41000000
        && DAT_0011d244 == 0
        && DAT_0011d23c == 2
        && DAT_0011fac8 == 1
        && cleanup_calls == 0
        && call_site()
        && DAT_001203a8 == -2091 && DAT_0012039c == -2091
        && DAT_001203b0 == 3984 && DAT_001203a4 == 3984
        && near_float(DAT_00120388, 0.4833219647407532f, 0.00001f)
        && near_float(DAT_00120394, 0.4833219647407532f, 0.00001f)
        && DAT_001203ac == 0x708 && DAT_001203a0 == 0x708
        && bits(DAT_00120384) == 0xbf20d97c
        && bits(DAT_0012038c) == 0
        && bits(DAT_0012037c) == 0
        && DAT_00109a0 == 157 && DAT_00109a8 == 0
        && bits(DAT_00118e08) == 0x3da3d70a
        && DAT_000007f9 == 8
        && records() ? '0' : 'x');

    reset('Z');
    name_entry_screen();
    put(DAT_0011d23c == 25
        && call_site()
        && DAT_001203a8 == 1076 && DAT_0012039c == 1076
        && DAT_001203b0 == 4369 && DAT_001203a4 == 4369
        && near_float(DAT_00120388, 6.041524410247803f, 0.00001f)
        && near_float(DAT_00120394, 6.041524410247803f, 0.00001f)
        && DAT_00109a0 == 1969
        && records() ? '1' : 'x');

    reset('A');
    name_entry_screen();
    put(DAT_0011d23c == 0
        && call_site()
        && bits(DAT_00120388) == 0
        && bits(DAT_00120394) == 0
        && DAT_001203a8 == 0 && DAT_0012039c == 0
        && DAT_001203b0 == 0x1194 && DAT_001203a4 == 0x1194
        && DAT_001203ac == 0x708 && DAT_001203a0 == 0x708
        && bits(DAT_00120384) == 0xbf20d97c
        && DAT_00109a0 == 0
        && records() ? '2' : 'x');

    reset('B');
    pcx_result = 1;
    name_entry_screen();
    put(cleanup_calls == 1
        && call_site()
        && DAT_0011d23c == 1
        && DAT_001203a8 == -1076 && DAT_001203b0 == 4369
        && DAT_00109a0 == 78
        && records() ? '3' : 'x');

    reset('C');
    name_entry_screen();
    put(record_int(1, 4) == 0x42
        && record_int(24, 4) == 0x59
        && record_int(25, 4) == 0x5a
        && record_int(26, 4) == 0
        && record_int(26, 8) == 0
        && record_int(26, 0xc) == 0
        && record_int(26, 0x38) == 0
        && record_int(26, 0x3c) == 0
        && record_int(26, 0x60) == 0 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
