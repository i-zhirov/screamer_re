/* The rebuilt `list_file_load` module.
 *
 * The original sits at linear 0x15460 in STARTH.EXE and holds 161 instructions
 * and 581 bytes: the three register saves, the frame, the 0x5a0-byte local
 * area, the two `sprintf` calls at `0x4d32c`, the member open at `0x4c08d`, the
 * error report at `0x4bf88`, the exit at `0x300be`, the four text reads at
 * `0x4c695`, the three `stricmp` calls at `0x4d9ba`, the `sscanf` call at
 * `0xc2024`, the `atoi` call at `0xc2045`, the `atof` call at `0xc209c`, the
 * inline string copy, the stream close at `0x4c38d`, and the epilogue.
 *
 * The function builds `%s\data\%s.dat` from the base path at `DAT_00007edc`
 * and the type, and the member name `%s.dtr` from the name.  It opens the path
 * as the member with the category `rt`.  A failed open reports the file error
 * and exits with 1.
 *
 * The function reads whole lines until the list name, then reads tokens until
 * `END_LIST`.  It skips the field line that starts with `*`.  The first byte of
 * a field selects the type: `d` an integer through `atoi`, `f` a float through
 * `atof`, `x` a hex integer through `sscanf` with `%x`, and `s` a string of
 * 0xc bytes.  The other bytes are separators.
 *
 * The store index is `row * stride + column`.  A full record advances the row
 * and clears the column.  The function closes the stream and returns the row
 * count.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The `-oi`
 * flag expands the `strcpy` call into the same two-byte copy loop as the
 * original.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness build defines `LIST_FILE_LOAD_HARNESS`, so the function builds the
 * constant text in local buffers.  The default build keeps the literals of the
 * original for the later period build.
 */
#include <string.h>

struct stream {
    char data[0xb4];
};

extern char DAT_00007edc[];
extern int sprintf(char *buffer, const char *format, ...);
extern int fopen(void *stream, char *path, char *member, char *category);
extern void file_error_report(void *stream);
extern void exit(int code);
extern int stream_read_text(void *stream, char *buffer, int mode);
extern int stricmp(const char *left, const char *right);
extern int sscanf(char *text, char *format, ...);
extern int atoi(char *text);
extern double atof(char *text);
extern int stream_close(void *stream);

#ifdef LIST_FILE_LOAD_HARNESS
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

int list_file_load(char *type, char *name, char *list, void *table,
                   int stride, int count)
{
#ifdef LIST_FILE_LOAD_HARNESS
    char path_format[0x14];
    char member_format[0x10];
    char mode_text[0x10];
    char end_text[0x10];
    char star_text[0x10];
    char hex_format[0x10];
#else
    char *path_format = "%s\\data\\%s.dat";
    char *member_format = "%s.dtr";
    char *mode_text = "rt";
    char *end_text = "END_LIST";
    char *star_text = "*";
    char *hex_format = "%x";
#endif
    struct stream file;
    char path[0x64];
    char member[0x64];
    char line[0x400];
    char *destination;
    int row;
    int column;
    int value;
    float number;

#ifdef LIST_FILE_LOAD_HARNESS
    fill(path_format, '%', 's', '\\', 'd', 'a', 't', 'a', '\\');
    fill(path_format + 8, '%', 's', '.', 'd', 'a', 't', 0, 0);
    fill(member_format, '%', 's', '.', 'd', 't', 'r', 0, 0);
    fill(mode_text, 'r', 't', 0, 0, 0, 0, 0, 0);
    fill(end_text, 'E', 'N', 'D', '_', 'L', 'I', 'S', 'T');
    fill(star_text, '*', 0, 0, 0, 0, 0, 0, 0);
    fill(hex_format, '%', 'x', 0, 0, 0, 0, 0, 0);
#endif

    sprintf(path, path_format, DAT_00007edc, type);
    sprintf(member, member_format, name);
    if (fopen(&file, path, member, mode_text) != 0) {
        file_error_report(&file);
        exit(1);
    }
    destination = table;
    row = 0;
    column = 0;
    while (stream_read_text(&file, line, 1) == 0) {
        if (stricmp(line, list) == 0) {
            break;
        }
    }
    for (;;) {
        if (stream_read_text(&file, line, 0) != 0) {
            break;
        }
        if (stricmp(line, end_text) == 0) {
            break;
        }
        if (stricmp(line, star_text) == 0) {
            stream_read_text(&file, line, 1);
            continue;
        }
        switch (line[0]) {
        case 'x':
            sscanf(line + 1, hex_format, &value);
            *(int *)(destination + row * stride + column) = value;
            column = column + 4;
            break;
        case 'd':
            value = atoi(line + 1);
            *(int *)(destination + row * stride + column) = value;
            column = column + 4;
            break;
        case 'f':
            number = atof(line + 1);
            *(float *)(destination + row * stride + column) = number;
            column = column + 4;
            break;
        case 's':
            strcpy(destination + row * stride + column, line + 1);
            column = column + 0xc;
            break;
        default:
            break;
        }
        if (column == count) {
            column = 0;
            row = row + 1;
        }
    }
    stream_close(&file);
    return row;
}
