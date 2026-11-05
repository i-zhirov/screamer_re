/* The test harness for the rebuilt `music_bank_load` module.
 *
 * The harness holds the bank record, the four globals of the module, the
 * 0x24-byte state buffer, and the stubs.  The stubs record their arguments and
 * return the configured results.  The harness runs seven paths:
 *
 *     0  the success path with no driver install: the status 0 and the active
 *        flag 1
 *     1  the sound-system failure sets the status 1 and runs no other call
 *     2  the driver-open failure sets the status 2, clears the handle, and
 *        runs the cleanup
 *     3  the driver value 0xa002 with one install failure sets the status 3
 *        and releases the two resources
 *     4  the driver value 0xa009 with two installs passes
 *     5  a zero melodic resource reports the memory error and passes
 *     6  a failed melodic open reports the file error and passes
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int music_bank_load(void *bank, int mode, int size, char *name, int resource);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char bank[0x100];
static char name_buffer[0x20];

char sound_state[0x24];
int music_melodic_data;
int music_melodic_size;
int music_drum_data;
int music_drum_size;
int sound_handles[5];

static int system_init_result;
static int driver_load_result;
static int fopen_result[2];
static int status_value;
static char *resource_result[2];
static int install_result[2];

static int system_init_calls;
static int memset_calls;
static void *memset_dest;
static int memset_value;
static unsigned int memset_count;

static int driver_load_calls;
static int driver_load_index;
static int *driver_load_record;
static int *driver_load_value;
static int driver_load_value_seen;
static void *driver_load_state;
static int *driver_load_bank;

static int cleanup_calls;
static int fopen_calls;
static char *fopen_path[2];
static char *fopen_member[2];
static char *fopen_category[2];
static void *fopen_stream[2];

static int file_error_calls;
static void *file_error_stream;

static int status_calls;

static int resource_read_calls;
static void *resource_read_resource;
static char *resource_read_name[2];
static int resource_read_size[2];

static int memory_error_calls;

static int stream_read_calls;
static void *stream_read_buffer[2];
static int stream_read_size[2];

static int stream_close_calls;

static int install_calls;
static int install_driver[2];
static int install_flag[2];
static void *install_data[2];

static int release_calls;
static char *release_name[2];

static int timer_calls;
static int timer_value;

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

static void reset(void)
{
    int index;

    for (index = 0; index < 0x100; ++index) {
        bank[index] = 0;
    }
    for (index = 0; index < 0x20; ++index) {
        name_buffer[index] = 0;
    }
    for (index = 0; index < 0x24; ++index) {
        sound_state[index] = 0x5a;
    }
    music_melodic_data = 0;
    music_melodic_size = 0;
    music_drum_data = 0;
    music_drum_size = 0;
    for (index = 0; index < 5; ++index) {
        sound_handles[index] = 0x77;
    }
    system_init_result = 0;
    driver_load_result = 0;
    fopen_result[0] = 0;
    fopen_result[1] = 0;
    status_value = 0x40;
    resource_result[0] = (char *)0x1234;
    resource_result[1] = (char *)0x5678;
    install_result[0] = 0;
    install_result[1] = 0;
    system_init_calls = 0;
    memset_calls = 0;
    memset_dest = 0;
    memset_value = 0;
    memset_count = 0;
    driver_load_calls = 0;
    driver_load_index = 0;
    driver_load_record = 0;
    driver_load_value = 0;
    driver_load_value_seen = 0;
    driver_load_state = 0;
    driver_load_bank = 0;
    cleanup_calls = 0;
    fopen_calls = 0;
    fopen_path[0] = 0;
    fopen_path[1] = 0;
    fopen_member[0] = 0;
    fopen_member[1] = 0;
    fopen_category[0] = 0;
    fopen_category[1] = 0;
    fopen_stream[0] = 0;
    fopen_stream[1] = 0;
    file_error_calls = 0;
    file_error_stream = 0;
    status_calls = 0;
    resource_read_calls = 0;
    resource_read_resource = 0;
    resource_read_name[0] = 0;
    resource_read_name[1] = 0;
    resource_read_size[0] = 0;
    resource_read_size[1] = 0;
    memory_error_calls = 0;
    stream_read_calls = 0;
    stream_read_buffer[0] = 0;
    stream_read_buffer[1] = 0;
    stream_read_size[0] = 0;
    stream_read_size[1] = 0;
    stream_close_calls = 0;
    install_calls = 0;
    install_driver[0] = 0;
    install_driver[1] = 0;
    install_flag[0] = 0;
    install_flag[1] = 0;
    install_data[0] = 0;
    install_data[1] = 0;
    release_calls = 0;
    release_name[0] = 0;
    release_name[1] = 0;
    timer_calls = 0;
    timer_value = 0;
}

static void set_name(char *value)
{
    int index;

    for (index = 0; value[index] != 0; ++index) {
        name_buffer[index] = value[index];
    }
    name_buffer[index] = 0;
}

int sound_system_init(int a, int b, int c)
{
    system_init_calls = system_init_calls + 1;
    return system_init_result;
}

void *memset(void *dest, int value, unsigned int count)
{
    unsigned char *bytes;
    unsigned int index;

    memset_calls = memset_calls + 1;
    memset_dest = dest;
    memset_value = value;
    memset_count = count;
    bytes = (unsigned char *)dest;
    for (index = 0; index < count; ++index) {
        bytes[index] = (unsigned char)value;
    }
    return dest;
}

int sound_driver_load(int index, int *record, int *value, void *state,
                      int *driver_bank)
{
    driver_load_calls = driver_load_calls + 1;
    driver_load_index = index;
    driver_load_record = record;
    driver_load_value = value;
    driver_load_value_seen = *value;
    driver_load_state = state;
    driver_load_bank = driver_bank;
    return driver_load_result;
}

void sound_system_cleanup(void)
{
    cleanup_calls = cleanup_calls + 1;
}

int fopen(void *stream, char *path, char *member, char *category)
{
    if (fopen_calls < 2) {
        fopen_stream[fopen_calls] = stream;
        fopen_path[fopen_calls] = path;
        fopen_member[fopen_calls] = member;
        fopen_category[fopen_calls] = category;
    }
    fopen_calls = fopen_calls + 1;
    if (fopen_calls <= 2) {
        return fopen_result[fopen_calls - 1];
    }
    return 0;
}

void file_error_report(void *stream)
{
    file_error_calls = file_error_calls + 1;
    file_error_stream = stream;
}

int stream_status(void *stream)
{
    status_calls = status_calls + 1;
    return status_value;
}

void *resource_read(void *resource, char *name, int size)
{
    if (resource_read_calls < 2) {
        resource_read_name[resource_read_calls] = name;
        resource_read_size[resource_read_calls] = size;
    }
    resource_read_resource = resource;
    resource_read_calls = resource_read_calls + 1;
    if (resource_read_calls <= 2) {
        return resource_result[resource_read_calls - 1];
    }
    return 0;
}

void memory_error_report(void *resource)
{
    memory_error_calls = memory_error_calls + 1;
}

int stream_read(void *stream, void *buffer, int size)
{
    if (stream_read_calls < 2) {
        stream_read_buffer[stream_read_calls] = buffer;
        stream_read_size[stream_read_calls] = size;
    }
    stream_read_calls = stream_read_calls + 1;
    return size;
}

int stream_close(void *stream)
{
    stream_close_calls = stream_close_calls + 1;
    return 0;
}

int sound_bank_install(int driver, int flag, void *data)
{
    if (install_calls < 2) {
        install_driver[install_calls] = driver;
        install_flag[install_calls] = flag;
        install_data[install_calls] = data;
    }
    install_calls = install_calls + 1;
    if (install_calls <= 2) {
        return install_result[install_calls - 1];
    }
    return 0;
}

void resource_release(void *resource, char *name)
{
    if (release_calls < 2) {
        release_name[release_calls] = name;
    }
    release_calls = release_calls + 1;
}

void sound_timer_set(int value)
{
    timer_calls = timer_calls + 1;
    timer_value = value;
}

static int success_calls(void)
{
    return system_init_calls == 1
        && memset_calls == 1
        && memset_dest == sound_state
        && memset_value == 0
        && memset_count == 0x24
        && driver_load_calls == 1
        && driver_load_record == (int *)(bank + 8)
        && driver_load_bank == (int *)(bank + 8)
        && driver_load_value_seen == 0x30
        && driver_load_state == sound_state
        && fopen_calls == 2
        && same(fopen_path[0], name_buffer)
        && same(fopen_path[1], name_buffer)
        && same(fopen_member[0], "melodic.bnk")
        && same(fopen_member[1], "drum.bnk")
        && same(fopen_category[0], "rb")
        && same(fopen_category[1], "rb")
        && status_calls == 6
        && resource_read_calls == 2
        && resource_read_resource == (void *)0x5555
        && same(resource_read_name[0], "MELODIC_BANK")
        && same(resource_read_name[1], "DRUM_BANK")
        && resource_read_size[0] == 0x40
        && resource_read_size[1] == 0x40
        && stream_read_calls == 2
        && stream_read_buffer[0] == (void *)0x1234
        && stream_read_buffer[1] == (void *)0x5678
        && stream_read_size[0] == 0x40
        && stream_read_size[1] == 0x40
        && stream_close_calls == 2
        && music_melodic_data == 0x1234
        && music_drum_data == 0x5678
        && music_melodic_size == 0x40
        && music_drum_size == 0x40;
}

static int bank_ok(int resource)
{
    return *(int *)(bank + 4) == -1
        && *(int *)(bank + 0x98) == resource
        && same((char *)(bank + 0x18), name_buffer);
}

int main(void)
{
    int result;

    reset();
    *(int *)(bank + 0x10) = 0;
    set_name("TESTPATH");
    result = music_bank_load(bank, 0x1234, 0x30, name_buffer, 0x5555);
    put(bank_ok(0x5555)
        && result == 0
        && *(int *)(bank + 0x10) == 0
        && *(int *)(bank + 0x14) == 1
        && success_calls()
        && timer_calls == 1
        && timer_value == 0x7f
        && memory_error_calls == 0
        && file_error_calls == 0 ? '0' : 'x');

    reset();
    *(int *)(bank + 0x10) = 0;
    set_name("TESTPATH");
    system_init_result = 1;
    result = music_bank_load(bank, 0x1234, 0x30, name_buffer, 0x5555);
    put(bank_ok(0x5555)
        && result == 1
        && *(int *)(bank + 0x10) == 1
        && system_init_calls == 1
        && memset_calls == 0
        && driver_load_calls == 0
        && fopen_calls == 0 ? '1' : 'x');

    reset();
    *(int *)(bank + 0x10) = 0;
    set_name("TESTPATH");
    driver_load_result = 1;
    result = music_bank_load(bank, 0x1234, 0x30, name_buffer, 0x5555);
    put(bank_ok(0x5555)
        && result == 2
        && *(int *)(bank + 0x10) == 2
        && sound_handles[0] == 0
        && cleanup_calls == 1
        && fopen_calls == 0
        && *(int *)(bank + 0x14) == 0 ? '2' : 'x');

    reset();
    *(int *)(bank + 0x10) = 0;
    set_name("TESTPATH");
    install_result[0] = 1;
    install_result[1] = 0;
    result = music_bank_load(bank, 0xa002, 0x30, name_buffer, 0x5555);
    put(bank_ok(0x5555)
        && result == 3
        && *(int *)(bank + 0x10) == 3
        && install_calls == 2
        && install_driver[0] == *(int *)(bank + 8)
        && install_flag[0] == 1
        && install_data[0] == (void *)0x1234
        && install_data[1] == (void *)0x5678
        && release_calls == 2
        && same(release_name[0], "MELODIC_BANK")
        && same(release_name[1], "DRUM_BANK")
        && sound_handles[0] == 0
        && cleanup_calls == 1
        && timer_calls == 0
        && *(int *)(bank + 0x14) == 0 ? '3' : 'x');

    reset();
    *(int *)(bank + 0x10) = 0;
    set_name("TESTPATH");
    result = music_bank_load(bank, 0xa009, 0x30, name_buffer, 0x5555);
    put(bank_ok(0x5555)
        && result == 0
        && *(int *)(bank + 0x10) == 0
        && *(int *)(bank + 0x14) == 1
        && success_calls()
        && install_calls == 2
        && timer_calls == 1
        && release_calls == 0
        && cleanup_calls == 0 ? '4' : 'x');

    reset();
    *(int *)(bank + 0x10) = 0;
    set_name("TESTPATH");
    resource_result[0] = 0;
    result = music_bank_load(bank, 0x1234, 0x30, name_buffer, 0x5555);
    put(bank_ok(0x5555)
        && result == 0
        && *(int *)(bank + 0x10) == 0
        && *(int *)(bank + 0x14) == 1
        && memory_error_calls == 1
        && stream_read_buffer[0] == 0
        && music_melodic_data == 0
        && music_drum_data == 0x5678 ? '5' : 'x');

    reset();
    *(int *)(bank + 0x10) = 0;
    set_name("TESTPATH");
    fopen_result[0] = -1;
    result = music_bank_load(bank, 0x1234, 0x30, name_buffer, 0x5555);
    put(bank_ok(0x5555)
        && result == 0
        && *(int *)(bank + 0x10) == 0
        && *(int *)(bank + 0x14) == 1
        && file_error_calls == 1
        && file_error_stream == (void *)fopen_stream[0]
        && fopen_calls == 2 ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
