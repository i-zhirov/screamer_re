/* The test harness for the rebuilt `engine_init` module.
 *
 * The harness holds the flag word, the two setup words, the engine list, the
 * three rectangle words, and the sixteen callee stubs.  Each stub records its
 * call and its arguments.  The `sprintf` stub writes `PATH` into the
 * destination buffer.  The harness runs four paths:
 *
 *     0  the setup word is zero: the sound and music block stays unrun, the
 *        other subsystem calls run once each, the flag is clear, and the
 *        rectangle holds 0, 0, and 0x280
 *     1  the setup word is set and the DPMI result differs from 3: the sound
 *        driver stops, the path builds, the music bank loads with the engine
 *        list, the status is zero, and `music_start` stays unrun
 *     2  the setup block with a non-zero music status: `music_start` runs with
 *        the bank struct
 *     3  the setup word is set and the DPMI result is 3: the sound driver stays
 *        unrun, but the path and the music bank still run
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

void engine_init(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0016684;
int DAT_00008070;
int DAT_00008078;
int DAT_000062ac;
int DAT_0015444;
int DAT_0015448;
int DAT_001544c;

static char text[8];

static int config_calls;
static int list_calls;
static char *list_type;
static char *list_name;
static char *list_record;
static void *list_table;
static int list_stride;
static int list_count;
static int setup_calls;
static int controls_calls;
static int mouse_calls;
static int dpmi_calls;
static int dpmi_result;
static int sound_calls;
static int sound_arg;
static int sprintf_calls;
static char *sprintf_buffer;
static char *sprintf_format;
static char *sprintf_value;
static int music_load_calls;
static void *music_bank;
static int music_mode;
static int music_size;
static char *music_name;
static int music_resource;
static int music_result;
static int music_start_calls;
static void *music_start_bank;
static int palette_calls;
static int keyboard_calls;
static int keyboard_arg;
static int interface_calls;
static void *interface_records;
static void *interface_state;
static int stream_calls;
static void *stream_ptr;
static int video_calls;
static int vesa_calls;
static int vesa_arg;

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

void config_load(void)
{
    config_calls = config_calls + 1;
}

void list_file_load(char *type, char *name, char *record, void *table,
                    int stride, int count)
{
    list_calls = list_calls + 1;
    list_type = type;
    list_name = name;
    list_record = record;
    list_table = table;
    list_stride = stride;
    list_count = count;
}

void setup_load(void)
{
    setup_calls = setup_calls + 1;
}

void controls_load(void)
{
    controls_calls = controls_calls + 1;
}

void mouse_init(void)
{
    mouse_calls = mouse_calls + 1;
}

int dpmi_real_int(void)
{
    dpmi_calls = dpmi_calls + 1;
    return dpmi_result;
}

void sound_driver_set(int value)
{
    sound_calls = sound_calls + 1;
    sound_arg = value;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *value;

    va_start(arguments, format);
    value = va_arg(arguments, char *);
    va_end(arguments);
    sprintf_calls = sprintf_calls + 1;
    sprintf_buffer = buffer;
    sprintf_format = (char *)format;
    sprintf_value = value;
    copy(buffer, "PATH");
    return 4;
}

int music_bank_load(void *bank, int mode, int size, char *name, int resource)
{
    music_load_calls = music_load_calls + 1;
    music_bank = bank;
    music_mode = mode;
    music_size = size;
    music_name = name;
    music_resource = resource;
    return music_result;
}

void music_start(void *bank)
{
    music_start_calls = music_start_calls + 1;
    music_start_bank = bank;
}

void palette_build(void)
{
    palette_calls = palette_calls + 1;
}

void keyboard_hook(int value)
{
    keyboard_calls = keyboard_calls + 1;
    keyboard_arg = value;
}

void engine_interface_init(void *records, void *state)
{
    interface_calls = interface_calls + 1;
    interface_records = records;
    interface_state = state;
}

void stream_layer_init(void *stream)
{
    stream_calls = stream_calls + 1;
    stream_ptr = stream;
}

void video_buf_load(void)
{
    video_calls = video_calls + 1;
}

void vesa_set_mode(int mode)
{
    vesa_calls = vesa_calls + 1;
    vesa_arg = mode;
}

static void reset(void)
{
    DAT_0016684 = 0x5a;
    DAT_00008070 = 0;
    DAT_00008078 = 0;
    DAT_000062ac = 0;
    DAT_0015444 = 0x5a;
    DAT_0015448 = 0x5a;
    DAT_001544c = 0x5a;
    config_calls = 0;
    list_calls = 0;
    list_type = 0;
    list_name = 0;
    list_record = 0;
    list_table = 0;
    list_stride = 0;
    list_count = 0;
    setup_calls = 0;
    controls_calls = 0;
    mouse_calls = 0;
    dpmi_calls = 0;
    dpmi_result = 3;
    sound_calls = 0;
    sound_arg = -1;
    sprintf_calls = 0;
    sprintf_buffer = 0;
    sprintf_format = 0;
    sprintf_value = 0;
    music_load_calls = 0;
    music_bank = 0;
    music_mode = -1;
    music_size = -1;
    music_name = 0;
    music_resource = 0;
    music_result = 0;
    music_start_calls = 0;
    music_start_bank = 0;
    palette_calls = 0;
    keyboard_calls = 0;
    keyboard_arg = -1;
    interface_calls = 0;
    interface_records = 0;
    interface_state = 0;
    stream_calls = 0;
    stream_ptr = 0;
    video_calls = 0;
    vesa_calls = 0;
    vesa_arg = -1;
}

static int common(void)
{
    return config_calls == 1
        && list_calls == 1
        && list_type == (char *)0x437
        && list_name == (char *)0x432
        && list_record == (char *)0x428
        && list_table == (void *)0x1f00
        && list_stride == 4
        && list_count == 4
        && setup_calls == 1
        && controls_calls == 1
        && mouse_calls == 1
        && dpmi_calls == 1
        && palette_calls == 1
        && keyboard_calls == 1
        && keyboard_arg == 1
        && interface_calls == 1
        && interface_records == (void *)0x6530
        && interface_state == (void *)0x6350
        && stream_calls == 1
        && stream_ptr == (void *)DAT_000062ac
        && video_calls == 1
        && vesa_calls == 1
        && vesa_arg == 1
        && DAT_001544c == 0
        && DAT_0015444 == 0
        && DAT_0015448 == 0x280;
}

static int setup_block(int sound_expected, int start_expected)
{
    return sound_calls == sound_expected
        && sprintf_calls == 1
        && sprintf_format == (char *)0x43b
        && sprintf_value == (char *)0x7f44
        && music_load_calls == 1
        && music_bank == (void *)0x62b0
        && music_mode == 5
        && music_size == 7
        && music_name == sprintf_buffer
        && same(music_name, "PATH")
        && music_resource == 0x13579
        && DAT_0016684 == 0
        && music_start_calls == start_expected
        && (start_expected == 0 || music_start_bank == (void *)0x62b0);
}

int main(void)
{
    reset();
    DAT_00008070 = 0;
    DAT_000062ac = 0x13579;
    dpmi_result = 3;
    engine_init();
    put(common()
        && DAT_0016684 == 0
        && sound_calls == 0
        && sprintf_calls == 0
        && music_load_calls == 0
        && music_start_calls == 0 ? '0' : 'x');

    reset();
    DAT_00008070 = 5;
    DAT_00008078 = 7;
    DAT_000062ac = 0x13579;
    dpmi_result = 0;
    engine_init();
    put(common()
        && sound_calls == 1
        && sound_arg == 0
        && setup_block(1, 0) ? '1' : 'x');

    reset();
    DAT_00008070 = 5;
    DAT_00008078 = 7;
    DAT_000062ac = 0x13579;
    dpmi_result = 0;
    music_result = 3;
    engine_init();
    put(common()
        && sound_calls == 1
        && sound_arg == 0
        && setup_block(1, 1) ? '2' : 'x');

    reset();
    DAT_00008070 = 5;
    DAT_00008078 = 7;
    DAT_000062ac = 0x13579;
    dpmi_result = 3;
    engine_init();
    put(common()
        && sound_calls == 0
        && setup_block(0, 0) ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
