/* The test harness for the rebuilt `cache_resource` module.
 *
 * The harness holds the resource structure, one record, and a `sprintf` stub.
 * The stub records its buffer, its format, and its name argument, and it copies
 * the name to the buffer.  The harness runs these paths:
 *
 *     0  a null record array is the failure path: the status becomes 1, the
 *        name goes to `resource + 0x10`, and the return is 0
 *     1  a null allocation result is the failure path with the same writes
 *     2  a valid record caches the allocation result at `+0x0c`, the size at
 *        `+0x14`, and the name at `+0x00`; it clears the status and increments
 *        the count
 *     3  a second record and a different size reach the same cache path
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

void *cache_resource(void *resource, char *record, void *value, char *name,
                     int size);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char resource[0x20];
static unsigned char record[0x1c];
static char name[8];
static void *sprintf_buffer;
static char *sprintf_format;
static char *sprintf_name;
static int sprintf_calls;

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

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *value;

    va_start(arguments, format);
    value = va_arg(arguments, char *);
    va_end(arguments);
    sprintf_buffer = buffer;
    sprintf_format = (char *)format;
    sprintf_name = value;
    copy(buffer, value);
    sprintf_calls = sprintf_calls + 1;
    return 0;
}

static void reset(int has_array, int count, int status)
{
    int index;

    for (index = 0; index < 0x20; ++index) {
        resource[index] = 0;
    }
    for (index = 0; index < 0x1c; ++index) {
        record[index] = 0;
    }
    *(void **)resource = has_array ? record : 0;
    *(int *)(resource + 4) = count;
    *(int *)(resource + 8) = 8;
    *(int *)(resource + 0xc) = status;
    name[0] = 'T';
    name[1] = 'E';
    name[2] = 'S';
    name[3] = 'T';
    name[4] = 0;
    sprintf_buffer = 0;
    sprintf_format = 0;
    sprintf_name = 0;
    sprintf_calls = 0;
}

int main(void)
{
    void *result;
    void *value;

    reset(0, 0, 5);
    result = cache_resource(resource, (char *)record, (void *)0x12345678, name,
                            0x100);
    put(result == 0 && *(int *)(resource + 0xc) == 1
        && sprintf_calls == 1 && sprintf_buffer == (char *)resource + 0x10
        && sprintf_name == name && same(sprintf_name, name)
        && *(int *)(record + 0xc) == 0 ? '0' : 'x');

    reset(1, 0, 5);
    result = cache_resource(resource, (char *)record, 0, name, 0x100);
    put(result == 0 && *(int *)(resource + 0xc) == 1
        && sprintf_calls == 1 && sprintf_buffer == (char *)resource + 0x10
        && sprintf_name == name && same(sprintf_name, name)
        && *(int *)(record + 0xc) == 0 ? '1' : 'x');

    reset(1, 2, 9);
    value = (void *)0x12345678;
    result = cache_resource(resource, (char *)record, value, name, 0x100);
    put(result == value && *(int *)(record + 0xc) == (int)value
        && *(int *)(record + 0x14) == 0x100
        && *(int *)(resource + 0xc) == 0 && *(int *)(resource + 4) == 3
        && sprintf_calls == 1 && sprintf_buffer == (char *)record
        && sprintf_name == name && same((char *)record, name) ? '2' : 'x');

    reset(1, 0, 7);
    *(void **)resource = record + 4;
    value = (void *)0x0badf00d;
    result = cache_resource(resource, (char *)(record + 4), value, name,
                            0x20000);
    put(result == value && *(int *)(record + 4 + 0xc) == (int)value
        && *(int *)(record + 4 + 0x14) == 0x20000
        && *(int *)(resource + 0xc) == 0 && *(int *)(resource + 4) == 1
        && sprintf_buffer == (char *)(record + 4)
        && same((char *)(record + 4), name) ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
