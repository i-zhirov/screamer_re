/* The rebuilt `member_list_load` module.
 *
 * The original sits at linear 0x1b394 in STARTH.EXE and holds 166 instructions
 * and 502 bytes up to the `ret` at 0x1b589.  The record keeps the 504-byte
 * extent and sets no function size.
 *
 * The function builds `%s\data\local.dat` from the base path at `DAT_00007f44`
 * and the member name `%s.txt` from the first argument.  It opens the member
 * with the category `rt` through the member open at `0x4c08d`.  A failed open
 * runs the file error report at `0x4bf88`.
 *
 * The first pass reads the member text with the line mode 1 through `0x4c695`
 * and sums the byte size of every line that does not start with `*`.  It then
 * gets the `TEXT` resource of that size through `0x2eed0` into
 * `DAT_000062ac`.  A zero result runs the memory error report at `0x2edad`.
 *
 * The function rewinds the stream through `0x4c5c7`, copies every line that
 * does not start with `*` into the resource, stores its pointer in the second
 * argument, and terminates the pointer array with -1.  It closes the stream
 * through `0x4c38d` and returns the resource pointer.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The `-oi`
 * flag expands the `strcpy` call into the same two-byte copy loop as the
 * original.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness build defines `MEMBER_LIST_LOAD_HARNESS`, so the function builds the
 * constant text in local buffers.  The default build keeps the literals of the
 * original for the later period build.
 */
#include <string.h>

struct stream {
    char data[0xb4];
};

extern char DAT_00007f44[];
extern void *DAT_000062ac;
extern int sprintf(char *buffer, const char *format, ...);
extern int fopen(void *stream, char *path, char *member, char *category);
extern void file_error_report(void *stream);
extern int stream_read_text(void *stream, char *buffer, int mode);
extern int stream_seek(void *stream, int offset, int origin);
extern void *resource_get(void *resource, char *name, int size);
extern void memory_error_report(void *resource);
extern int stream_close(void *stream);

#ifdef MEMBER_LIST_LOAD_HARNESS
static void fill(char *text, char a, char b, char c, char d,
                 char e, char f, char g, char h)
{
    text[0] = a;
    text[1] = b;
    text[2] = c;
    text[3] = d;
    text[4] = e;
    text[5] = f;
    text[6] = g;
    text[7] = h;
    text[8] = 0;
}
#endif

static int text_length(char *text)
{
    int length;

    length = 0;
    while (text[length] != 0) {
        length = length + 1;
    }
    return length;
}

void *member_list_load(char *name, int *array)
{
#ifdef MEMBER_LIST_LOAD_HARNESS
    char path_format[0x14];
    char member_format[0x10];
    char list_name[0x10];
    char mode_text[0x10];
#else
    char *path_format = "%s\\data\\local.dat";
    char *member_format = "%s.txt";
    char *list_name = "TEXT";
    char *mode_text = "rt";
#endif
    struct stream file;
    char path[0x80];
    char member[0x10];
    char line[0x100];
    char *buffer;
    int total;
    int index;
    int offset;

#ifdef MEMBER_LIST_LOAD_HARNESS
    fill(path_format, '%', 's', '\\', 'd', 'a', 't', 'a', '\\');
    fill(path_format + 8, 'l', 'o', 'c', 'a', 'l', '.', 'd', 'a');
    path_format[16] = 't';
    path_format[17] = 0;
    fill(member_format, '%', 's', '.', 't', 'x', 't', 0, 0);
    fill(list_name, 'T', 'E', 'X', 'T', 0, 0, 0, 0);
    fill(mode_text, 'r', 't', 0, 0, 0, 0, 0, 0);
#endif

    sprintf(path, path_format, DAT_00007f44);
    sprintf(member, member_format, name);
    if (fopen(&file, path, member, mode_text) != 0) {
        file_error_report(&file);
    }
    total = 0;
    while (stream_read_text(&file, line, 1) == 0) {
        if (line[0] != '*') {
            total = total + text_length(line) + 1;
        }
    }
    buffer = resource_get(DAT_000062ac, list_name, total);
    if (buffer == 0) {
        memory_error_report(DAT_000062ac);
    }
    stream_seek(&file, 0, 0);
    index = 0;
    offset = 0;
    while (stream_read_text(&file, line, 1) == 0) {
        if (line[0] != '*') {
            strcpy(buffer + offset, line);
            array[index] = (int)(buffer + offset);
            index = index + 1;
            offset = offset + text_length(line) + 1;
        }
    }
    array[index] = -1;
    stream_close(&file);
    return buffer;
}
