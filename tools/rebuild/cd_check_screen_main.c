/* The test harness for the rebuilt `cd_check_screen` module.
 *
 * The harness holds the archive pointer, the two buffers, the palette, the
 * message text, the two audio flags, and the stubs of the nine callees.  The
 * `cd_check` stub returns a scripted sequence: 0 means the correct CD is
 * present, and 1 means it is missing or wrong.  The `cd_audio_poll` stub
 * toggles the audio flag, so every wait loop of the module ends.
 *
 * The harness runs five paths:
 *
 *     0  the first check returns 0: the module returns before any call
 *     1  the first check and the wait check fail once and then succeed: the
 *        full screen path runs, the audio flag toggles four times, and the
 *        module sets the word 0x1669e and resets the audio
 *     2  the resource result is zero: the memory error report runs and the
 *        path continues
 *     3  the PCX load fails: the cleanup at 0x15150 runs
 *     4  the wait check succeeds at once: the module polls only twice
 *     D  the exit marker
 *
 * The string arguments of the module literals are not readable in this minimal
 * image, so the stubs record only the pointers and the harness checks the
 * non-string arguments.
 */
#pragma off (check_stack)

void cd_check_screen(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_000062ac;
int DAT_000166ac;
void *DAT_0001fad0;
void *DAT_00021528;
char *DAT_00018ff4;
int DAT_00016688;
short DAT_0001669e;
char DAT_0000cf5c[0x300];

static char video_buffer[0x10];
static char pixel_buffer[0x10];
static char message[] = "Insert the Screamer CD";
static char text[8];

static int check_calls;
static int check_values[8];
static int check_total;

static int stop_calls;
static int resource_calls;
static void *resource_archive_arg;
static const char *resource_name_arg;
static int resource_size_arg;
static void *resource_result;

static int memory_error_calls;
static void *memory_error_arg;

static int memset_calls;
static void *memset_target;
static unsigned int memset_count;
static int memset_value;

static int video_fill_calls;

static int pcx_load_calls;
static const char *pcx_file_arg;
static const char *pcx_member_arg;
static void *pcx_pixels_arg;
static char *pcx_palette_arg;
static int pcx_result;

static int cleanup_calls;

static int memcpy_calls;
static void *memcpy_target;
static const void *memcpy_source;
static unsigned int memcpy_count;

static int panel_calls;
static const char *panel_text_arg;
static int panel_y_arg;
static int panel_font_arg;
static int panel_color_arg;

static int palette_calls;
static char *palette_arg;

static int sound_set_calls;
static int sound_set_value;

static int poll_calls;

static int reset_calls;

int cd_check(void)
{
    int value;

    value = check_values[check_calls < check_total ? check_calls
                                                   : check_total - 1];
    check_calls = check_calls + 1;
    return value;
}

void cd_audio_stop(void)
{
    stop_calls = stop_calls + 1;
}

void *resource_get(void *archive, const char *name, int size)
{
    resource_calls = resource_calls + 1;
    resource_archive_arg = archive;
    resource_name_arg = name;
    resource_size_arg = size;
    return resource_result;
}

void memory_error_report(void *archive)
{
    memory_error_calls = memory_error_calls + 1;
    memory_error_arg = archive;
}

void *memset(void *target, unsigned int count, int value)
{
    memset_calls = memset_calls + 1;
    memset_target = target;
    memset_count = count;
    memset_value = value;
    return target;
}

void video_fill(void)
{
    video_fill_calls = video_fill_calls + 1;
}

int pcx_load(const char *file, const char *member, void *pixels, char *palette)
{
    pcx_load_calls = pcx_load_calls + 1;
    pcx_file_arg = file;
    pcx_member_arg = member;
    pcx_pixels_arg = pixels;
    pcx_palette_arg = palette;
    return pcx_result;
}

void sub_15150(void)
{
    cleanup_calls = cleanup_calls + 1;
}

void *memcpy(void *target, const void *source, unsigned int count)
{
    memcpy_calls = memcpy_calls + 1;
    memcpy_target = target;
    memcpy_source = source;
    memcpy_count = count;
    return target;
}

void panel_draw_centered(const char *text_value, int y, int font, int color)
{
    panel_calls = panel_calls + 1;
    panel_text_arg = text_value;
    panel_y_arg = y;
    panel_font_arg = font;
    panel_color_arg = color;
}

void palette_set(char *palette)
{
    palette_calls = palette_calls + 1;
    palette_arg = palette;
}

void sound_driver_set(int value)
{
    sound_set_calls = sound_set_calls + 1;
    sound_set_value = value;
}

void cd_audio_poll(void)
{
    poll_calls = poll_calls + 1;
    DAT_00016688 = DAT_00016688 == 0 ? 1 : 0;
}

void cd_audio_reset(void)
{
    reset_calls = reset_calls + 1;
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

static void reset(void)
{
    check_calls = 0;
    check_total = 0;
    stop_calls = 0;
    resource_calls = 0;
    resource_archive_arg = 0;
    resource_name_arg = 0;
    resource_size_arg = 0;
    resource_result = pixel_buffer;
    memory_error_calls = 0;
    memory_error_arg = 0;
    memset_calls = 0;
    memset_target = 0;
    memset_count = 0;
    memset_value = 0;
    video_fill_calls = 0;
    pcx_load_calls = 0;
    pcx_file_arg = 0;
    pcx_member_arg = 0;
    pcx_pixels_arg = 0;
    pcx_palette_arg = 0;
    pcx_result = 0;
    cleanup_calls = 0;
    memcpy_calls = 0;
    memcpy_target = 0;
    memcpy_source = 0;
    memcpy_count = 0;
    panel_calls = 0;
    panel_text_arg = 0;
    panel_y_arg = 0;
    panel_font_arg = 0;
    panel_color_arg = 0;
    palette_calls = 0;
    palette_arg = 0;
    sound_set_calls = 0;
    sound_set_value = 0;
    poll_calls = 0;
    reset_calls = 0;

    DAT_000062ac = (void *)0x1234;
    DAT_000166ac = 0x55aa;
    DAT_0001fad0 = 0;
    DAT_00021528 = video_buffer;
    DAT_00018ff4 = message;
    DAT_00016688 = 0;
    DAT_0001669e = 0;
}

static void set_checks(int first, int second, int third, int total)
{
    check_values[0] = first;
    check_values[1] = second;
    check_values[2] = third;
    check_total = total;
}

int main(void)
{
    reset();
    set_checks(0, 0, 0, 1);
    cd_check_screen();
    put(check_calls == 1 && stop_calls == 0 && resource_calls == 0
        && memory_error_calls == 0 && memset_calls == 0
        && video_fill_calls == 0 && pcx_load_calls == 0 && cleanup_calls == 0
        && memcpy_calls == 0 && panel_calls == 0 && palette_calls == 0
        && sound_set_calls == 0 && poll_calls == 0 && reset_calls == 0
        && DAT_000166ac == 0x55aa && DAT_0001fad0 == 0
        && DAT_0001669e == 0 ? '0' : 'x');

    reset();
    set_checks(1, 1, 0, 3);
    cd_check_screen();
    put(check_calls == 3 && stop_calls == 1
        && resource_calls == 1
        && resource_archive_arg == (void *)0x1234
        && resource_size_arg == 0x3e800
        && memory_error_calls == 0
        && DAT_000166ac == 0
        && DAT_0001fad0 == pixel_buffer
        && memset_calls == 1 && memset_target == video_buffer
        && memset_count == 0x3e800 && memset_value == 0
        && video_fill_calls == 2
        && pcx_load_calls == 1 && pcx_pixels_arg == pixel_buffer
        && pcx_palette_arg == DAT_0000cf5c && cleanup_calls == 0
        && memcpy_calls == 1 && memcpy_target == video_buffer
        && memcpy_source == pixel_buffer && memcpy_count == 0x3e800
        && panel_calls == 1 && panel_text_arg == message
        && panel_y_arg == 0x3c && panel_font_arg == 0
        && panel_color_arg == 2
        && palette_calls == 1 && palette_arg == DAT_0000cf5c
        && sound_set_calls == 1 && sound_set_value == 0
        && poll_calls == 4 && DAT_0001669e == 1
        && reset_calls == 1 ? '1' : 'x');

    reset();
    set_checks(1, 1, 0, 3);
    resource_result = 0;
    cd_check_screen();
    put(memory_error_calls == 1
        && memory_error_arg == (void *)0x1234
        && DAT_0001fad0 == 0
        && resource_calls == 1
        && pcx_load_calls == 1 && pcx_pixels_arg == 0
        && memcpy_calls == 1 && memcpy_source == 0
        && panel_calls == 1 && reset_calls == 1 ? '2' : 'x');

    reset();
    set_checks(1, 1, 0, 3);
    pcx_result = 1;
    cd_check_screen();
    put(cleanup_calls == 1 && pcx_load_calls == 1
        && panel_calls == 1 && reset_calls == 1 ? '3' : 'x');

    reset();
    set_checks(1, 0, 0, 2);
    cd_check_screen();
    put(check_calls == 2 && poll_calls == 2
        && DAT_0001669e == 1 && reset_calls == 1 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
