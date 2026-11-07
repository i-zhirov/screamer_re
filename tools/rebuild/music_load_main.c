/* The test harness for the rebuilt `music_load` module.
 *
 * The harness holds the music record and the resource buffer.  The stubs record
 * their arguments and their call counts.  The `sprintf` stub builds the
 * `%s.HMP` name from the argument.  The `stream_status` stub returns the
 * configured size.
 *
 * The harness runs five paths:
 *
 *     0  the record gate is clear: no bank stop, no format, no open, result 0
 *     1  the full path: the name, the open, the resource, the read, the close,
 *        the 16-entry table, and the success flag
 *     2  the song loader fails: the resource name free and the result 4
 *     3  the open fails: the file error report runs
 *     4  the resource request fails: the memory error report runs
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

int music_load(char *record, char *name);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char record[0x100];
static char resource_block[0x100];

static int bank_calls;
static char *bank_record;
static int format_calls;
static char *format_buffer;
static char *format_text;
static char *format_name;
static int open_calls;
static char *open_stream;
static char *open_path;
static char *open_name;
static char *open_category;
static int open_result;
static int file_error_calls;
static int resource_calls;
static void *resource_argument;
static char *resource_name;
static int resource_size;
static void *resource_result;
static int memory_error_calls;
static int read_calls;
static char *read_stream;
static char *read_buffer;
static int read_count;
static int close_calls;
static int status_calls;
static int status_value;
static int song_calls;
static void *song_buffer;
static char *song_record;
static int *song_table;
static int song_table_ok;
static int song_result;
static int free_calls;
static void *free_resource;
static char *free_name;

static void put(char marker)
{
    text[0] = marker;
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

static void append(char *destination, char *suffix)
{
    int index;
    int offset;

    for (offset = 0; destination[offset] != 0; ++offset) {
    }
    for (index = 0; suffix[index] != 0; ++index) {
        destination[offset + index] = suffix[index];
    }
    destination[offset + index] = 0;
}

static void set_int(int offset, int value)
{
    *(int *)(record + offset) = value;
}

static int get_int(int offset)
{
    return *(int *)(record + offset);
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x100; index++) {
        record[index] = 0;
        resource_block[index] = 0;
    }
    bank_calls = 0;
    bank_record = 0;
    format_calls = 0;
    format_buffer = 0;
    format_text = 0;
    format_name = 0;
    open_calls = 0;
    open_stream = 0;
    open_path = 0;
    open_name = 0;
    open_category = 0;
    open_result = 0;
    file_error_calls = 0;
    resource_calls = 0;
    resource_argument = 0;
    resource_name = 0;
    resource_size = 0;
    resource_result = resource_block;
    memory_error_calls = 0;
    read_calls = 0;
    read_stream = 0;
    read_buffer = 0;
    read_count = 0;
    close_calls = 0;
    status_calls = 0;
    status_value = 0x40;
    song_calls = 0;
    song_buffer = 0;
    song_record = 0;
    song_table = 0;
    song_table_ok = 0;
    song_result = 0;
    free_calls = 0;
    free_resource = 0;
    free_name = 0;
}

void music_bank_stop(char *record_argument)
{
    bank_calls = bank_calls + 1;
    bank_record = record_argument;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *value;

    va_start(arguments, format);
    value = va_arg(arguments, char *);
    va_end(arguments);
    format_calls = format_calls + 1;
    format_buffer = buffer;
    format_text = (char *)format;
    format_name = value;
    copy(buffer, value);
    append(buffer, ".HMP");
    return 1;
}

int fopen(char *stream, char *path, char *name, char *category)
{
    open_calls = open_calls + 1;
    open_stream = stream;
    open_path = path;
    open_name = name;
    open_category = category;
    return open_result;
}

void file_error_report(char *stream)
{
    file_error_calls = file_error_calls + 1;
}

unsigned int stream_status(char *stream)
{
    status_calls = status_calls + 1;
    return (unsigned int)status_value;
}

void *resource_get(void *resource, char *name, int size)
{
    resource_calls = resource_calls + 1;
    resource_argument = resource;
    resource_name = name;
    resource_size = size;
    return resource_result;
}

void memory_error_report(void *resource)
{
    memory_error_calls = memory_error_calls + 1;
}

int stream_read(char *stream, char *buffer, int count)
{
    read_calls = read_calls + 1;
    read_stream = stream;
    read_buffer = buffer;
    read_count = count;
    return count;
}

int stream_close(char *stream)
{
    close_calls = close_calls + 1;
    return 0;
}

int midi_song_load(void *buffer, char *record_argument, int *table)
{
    int index;

    song_calls = song_calls + 1;
    song_buffer = buffer;
    song_record = record_argument;
    song_table = table;
    song_table_ok = 1;
    for (index = 0; index < 16; index++) {
        if (table[index] != -1) {
            song_table_ok = 0;
        }
    }
    return song_result;
}

void resource_free_name(void *resource, char *name)
{
    free_calls = free_calls + 1;
    free_resource = resource;
    free_name = name;
}

int main(void)
{
    int result;

    reset();
    set_int(0x10, 7);
    set_int(0x14, 0);
    result = music_load(record, "SONG");
    put(result == 0
            && get_int(0x10) == 0
            && bank_calls == 0
            && format_calls == 0
            && open_calls == 0
        ? '0' : 'x');

    reset();
    set_int(0x14, 1);
    set_int(0x98, (int)resource_block);
    status_value = 0x40;
    result = music_load(record, "SONG");
    put(result == 0
            && bank_calls == 1
            && bank_record == record
            && format_calls == 1
            && same(format_text, "%s.HMP")
            && same(format_name, "SONG")
            && same(format_buffer, "SONG.HMP")
            && open_calls == 1
            && open_path == record + 0x18
            && same(open_category, "rb")
            && same(open_name, "SONG.HMP")
            && resource_calls == 1
            && resource_argument == resource_block
            && same(resource_name, "MIDI_MUSIC")
            && resource_size == 0x40
            && read_calls == 1
            && read_buffer == resource_block
            && read_count == 0x40
            && close_calls == 1
            && song_calls == 1
            && song_buffer == resource_block
            && song_record == record
            && song_table_ok == 1
            && free_calls == 0
            && get_int(4) == 1
            && get_int(0x10) == 0
        ? '1' : 'x');

    reset();
    set_int(0x14, 1);
    set_int(0x98, (int)resource_block);
    song_result = 1;
    result = music_load(record, "SONG");
    put(result == 4
            && free_calls == 1
            && free_resource == resource_block
            && same(free_name, "MIDI_MUSIC")
            && get_int(4) == 0
            && get_int(0x10) == 4
        ? '2' : 'x');

    reset();
    set_int(0x14, 1);
    set_int(0x98, (int)resource_block);
    open_result = 1;
    result = music_load(record, "SONG");
    put(file_error_calls == 1
            && resource_calls == 1
            && read_calls == 1
            && song_calls == 1
            && result == 0
        ? '3' : 'x');

    reset();
    set_int(0x14, 1);
    set_int(0x98, (int)resource_block);
    resource_result = 0;
    result = music_load(record, "SONG");
    put(memory_error_calls == 1
            && read_calls == 1
            && read_buffer == 0
            && song_calls == 1
            && result == 0
        ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
