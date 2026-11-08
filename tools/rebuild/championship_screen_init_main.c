#pragma off (check_stack)

#include <stdarg.h>

void championship_screen_init(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0011d240;
float DAT_00120378;
float DAT_00120380;
int DAT_0011fac8;
int DAT_001201c0[16];
int DAT_0011d260;
int DAT_00120400[6];
int DAT_001203dc;
int DAT_001203e0;
char DAT_001022bc[4 * 0x2c];
void *DAT_0011fad0;
void *DAT_00121528;
unsigned char DAT_0010cf5c[0x300];
float DAT_00118e08;
int DAT_000007f9;
int DAT_0011d24c;
char championship_format[] = "%s%c.pcx";
char championship_archive[] = "gfx\\graf.dat";

char *DAT_00120030[8];

static char text[8];
static char pcx_file[32];
static char pcx_member[32];
static int pcx_calls;
static int pcx_result;
static void *pcx_pixels;
static char *pcx_palette;

static int run_calls;
static int cleanup_calls;
static int palette_calls;
static unsigned char *palette_pointer;
static int present_calls;
static float present_scale[4];
static int present_offset[4];
static int copy_calls;
static void *copy_target;
static void *copy_source;
static unsigned int copy_count;
static int fill_calls;

static char pixel_buffer[0x400];
static char screen_buffer[0x400];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void copy(char *destination, char *source)
{
    int index;

    for (index = 0; source[index] != 0 && index < 0x1f; index++) {
        destination[index] = source[index];
    }
    destination[index] = 0;
}

static int same(char *left, char *right)
{
    int index;

    for (index = 0; left[index] == right[index]; index++) {
        if (left[index] == 0) {
            return 1;
        }
    }
    return 0;
}

static int near_value(float left, float right, float tolerance)
{
    float difference;

    difference = left - right;
    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference < tolerance;
}

static void set_name(int index, char *name)
{
    copy(DAT_001022bc + index * 0x2c + 0x18, name);
}

static void reset(void)
{
    int index;

    DAT_0011d240 = 0;
    DAT_00120378 = 0.0f;
    DAT_00120380 = 0.0f;
    DAT_0011fac8 = 0;
    DAT_0011d260 = 0;
    DAT_001203dc = 0;
    DAT_001203e0 = 0;
    for (index = 0; index < 16; index++) {
        DAT_001201c0[index] = 0;
    }
    for (index = 0; index < 6; index++) {
        DAT_00120400[index] = 0;
    }
    for (index = 0; index < 4 * 0x2c; index++) {
        DAT_001022bc[index] = 0;
    }
    DAT_0011fad0 = pixel_buffer;
    DAT_00121528 = screen_buffer;
    DAT_00118e08 = 0.0f;
    DAT_000007f9 = 0;
    DAT_0011d24c = 0;

    pcx_calls = 0;
    pcx_result = 0;
    pcx_pixels = 0;
    pcx_palette = 0;
    pcx_file[0] = 0;
    pcx_member[0] = 0;
    run_calls = 0;
    cleanup_calls = 0;
    palette_calls = 0;
    palette_pointer = 0;
    present_calls = 0;
    copy_calls = 0;
    copy_target = 0;
    copy_source = 0;
    copy_count = 0;
    fill_calls = 0;
    for (index = 0; index < 4; index++) {
        present_scale[index] = 0.0f;
        present_offset[index] = 0;
    }
}

void sub_2dff4(void)
{
    run_calls = run_calls + 1;
}

void sub_15150(void)
{
    cleanup_calls = cleanup_calls + 1;
}

void palette_shade_build(unsigned char *palette)
{
    palette_calls = palette_calls + 1;
    palette_pointer = palette;
}

void video_present(float scale, int offset)
{
    if (present_calls < 4) {
        present_scale[present_calls] = scale;
        present_offset[present_calls] = offset;
    }
    present_calls = present_calls + 1;
}

void *memcpy(void *target, const void *source, unsigned int count)
{
    copy_calls = copy_calls + 1;
    copy_target = target;
    copy_source = (void *)source;
    copy_count = count;
    return target;
}

void video_fill(void)
{
    fill_calls = fill_calls + 1;
}

int pcx_load(char *file, char *member, void *pixels, char *palette)
{
    pcx_calls = pcx_calls + 1;
    copy(pcx_file, file);
    copy(pcx_member, member);
    pcx_pixels = pixels;
    pcx_palette = palette;
    return pcx_result;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *out;
    char *string;
    int index;
    char character;

    out = buffer;
    index = 0;
    va_start(arguments, format);
    while (format[index] != 0) {
        if (format[index] != '%') {
            *out = format[index];
            out = out + 1;
            index = index + 1;
            continue;
        }
        index = index + 1;
        if (format[index] == 's') {
            string = va_arg(arguments, char *);
            while (*string != 0) {
                *out = *string;
                out = out + 1;
                string = string + 1;
            }
            index = index + 1;
        } else if (format[index] == 'c') {
            character = (char)va_arg(arguments, int);
            *out = character;
            out = out + 1;
            index = index + 1;
        } else {
            *out = format[index];
            out = out + 1;
            index = index + 1;
        }
    }
    va_end(arguments);
    *out = 0;
    return out - buffer;
}

int main(void)
{
    reset();
    DAT_0011d260 = 0;
    DAT_001203dc = 0;
    set_name(0, "champ0");
    set_name(1, "champ5");
    set_name(2, "champ2");
    set_name(3, "champ9");
    DAT_00120400[0] = 9;
    DAT_00120400[1] = 9;
    DAT_00120400[2] = 9;
    DAT_00120400[3] = 9;
    DAT_00120400[4] = 9;
    DAT_00120400[5] = 9;
    DAT_00120030[0] = (char *)-1;
    championship_screen_init();
    put(DAT_0011d240 == 1
            && near_value(DAT_00120378, 1.5f, 0.001f)
            && near_value(DAT_00120380, 0.95f, 0.001f)
            && DAT_0011fac8 == 1
            && run_calls == 1
            && DAT_001201c0[0] == 0
            && DAT_001201c0[1] == 0
            && DAT_001201c0[2] == 0
            && DAT_001201c0[3] == 0
            && DAT_001201c0[4] == 0
            && DAT_00120400[0] == 9
            && DAT_00120400[5] == 9
            && DAT_001203dc == 0
            && DAT_001203e0 == 0
            && pcx_calls == 1
            && same(pcx_file, "gfx\\graf.dat")
            && same(pcx_member, "champ0H.pcx")
            && pcx_pixels == DAT_0011fad0
            && pcx_palette == (char *)DAT_0010cf5c
            && cleanup_calls == 0
            && near_value(DAT_00118e08, 0.08f, 0.001f)
            && DAT_000007f9 == 8
            && palette_calls == 1
            && palette_pointer == DAT_0010cf5c
            && present_calls == 2
            && near_value(present_scale[0], 0.0f, 0.001f)
            && near_value(present_scale[1], 0.0f, 0.001f)
            && present_offset[0] == 0
            && present_offset[1] == 0
            && copy_calls == 1
            && copy_target == DAT_00121528
            && copy_source == DAT_0011fad0
            && copy_count == 0x3e800
            && fill_calls == 1
            && DAT_0011d24c == 0
        ? '0' : 'x');

    reset();
    DAT_0011d260 = -1;
    DAT_001203dc = 2;
    DAT_001203e0 = 5;
    set_name(0, "champ0");
    set_name(1, "champ5");
    set_name(2, "champ2");
    DAT_00120400[0] = 9;
    DAT_00120400[1] = 9;
    DAT_00120400[2] = 9;
    DAT_00120400[3] = 9;
    DAT_00120400[4] = 9;
    DAT_00120400[5] = 9;
    DAT_00120030[0] = (char *)-1;
    championship_screen_init();
    put(DAT_00120400[0] == -1
            && DAT_00120400[1] == -1
            && DAT_00120400[2] == -1
            && DAT_00120400[3] == -1
            && DAT_00120400[4] == -1
            && DAT_00120400[5] == -1
            && DAT_001203dc == 0
            && DAT_001203e0 == 0
            && DAT_001201c0[0] == 0
            && DAT_001201c0[1] == 0
            && DAT_001201c0[2] == 0
            && DAT_001201c0[3] == 0
            && DAT_001201c0[4] == 0
            && same(pcx_member, "champ0H.pcx")
            && pcx_calls == 1
            && cleanup_calls == 0
        ? '1' : 'x');

    reset();
    DAT_0011d260 = 0;
    DAT_001203dc = 1;
    set_name(0, "champ0");
    set_name(1, "champ5");
    set_name(2, "champ2");
    pcx_result = 1;
    DAT_00120030[0] = (char *)-1;
    championship_screen_init();
    put(same(pcx_member, "champ5H.pcx")
            && pcx_calls == 1
            && cleanup_calls == 1
            && fill_calls == 1
        ? '2' : 'x');

    reset();
    DAT_0011d260 = 0;
    DAT_001203dc = 2;
    set_name(0, "champ0");
    set_name(1, "champ5");
    set_name(2, "champ2");
    DAT_00120030[0] = (char *)-1;
    championship_screen_init();
    put(same(pcx_member, "champ2H.pcx")
            && pcx_calls == 1
            && cleanup_calls == 0
        ? '3' : 'x');

    reset();
    DAT_0011d260 = 0;
    DAT_001203dc = 3;
    set_name(3, "champ9");
    DAT_00120030[0] = (char *)-1;
    championship_screen_init();
    put(DAT_001201c0[0] == 0
            && same(pcx_member, "champ9H.pcx")
            && pcx_calls == 1
            && cleanup_calls == 0
        ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
