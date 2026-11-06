/* The test harness for the rebuilt `resource_get` module.
 *
 * The harness holds the resource structure, the record array, and the four
 * stubs.  The stubs record their arguments.  The DOS allocation stub writes the
 * configured selector to the record cell and returns the configured address.
 * The small allocation stub writes the configured address to the record cell
 * and returns the configured value.  The cache stub returns the configured
 * value.  The harness runs five paths:
 *
 *     0  the full-table path sets the status to 2, copies the name to
 *        `resource + 0x10`, and returns 0 without an allocation or a cache
 *     1  the small-size DOS path sets the flag to 1 and caches the DOS result
 *     2  the failed DOS path falls back to the small allocation and clears the
 *        flag
 *     3  the size at the limit `0x96000` uses the small allocation directly
 *     4  the second-record path uses the 0x1c-byte stride and the same calls
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

void *resource_get(void *resource, char *name, int size);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char resource[0x20];
static unsigned char records[2 * 0x1c];
static char name[12];
static void *sprintf_buffer;
static char *sprintf_format;
static char *sprintf_name;
static int sprintf_calls;
static void *dos_handle;
static unsigned int dos_size;
static unsigned int *dos_selector;
static unsigned int dos_selector_value;
static unsigned int dos_result;
static int dos_calls;
static void *small_handle;
static int small_size;
static unsigned int *small_address;
static unsigned int small_address_value;
static int small_result;
static int small_calls;
static void *cache_handle;
static char *cache_record;
static void *cache_value;
static char *cache_name;
static int cache_size;
static void *cache_result;
static int cache_calls;

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

int dos_memory_alloc(void *handle, unsigned int size, unsigned int *selector)
{
    dos_handle = handle;
    dos_size = size;
    dos_selector = selector;
    if (dos_result != 0) {
        *selector = dos_selector_value;
    }
    dos_calls = dos_calls + 1;
    return (int)dos_result;
}

int memory_alloc_small(void *handle, int size, unsigned int *address)
{
    small_handle = handle;
    small_size = size;
    small_address = address;
    *address = small_address_value;
    small_calls = small_calls + 1;
    return small_result;
}

void *cache_resource(void *handle, char *record, void *value, char *value_name,
                     int size)
{
    cache_handle = handle;
    cache_record = record;
    cache_value = value;
    cache_name = value_name;
    cache_size = size;
    cache_calls = cache_calls + 1;
    return cache_result;
}

static void reset(int count, int limit)
{
    int index;

    for (index = 0; index < 0x20; ++index) {
        resource[index] = 0;
    }
    for (index = 0; index < 2 * 0x1c; ++index) {
        records[index] = 0;
    }
    *(void **)resource = records;
    *(int *)(resource + 4) = count;
    *(int *)(resource + 8) = limit;
    name[0] = 'T';
    name[1] = 'E';
    name[2] = 'S';
    name[3] = 'T';
    name[4] = 0;
    sprintf_buffer = 0;
    sprintf_format = 0;
    sprintf_name = 0;
    sprintf_calls = 0;
    dos_handle = 0;
    dos_size = 0;
    dos_selector = 0;
    dos_selector_value = 0x3333;
    dos_result = 0;
    dos_calls = 0;
    small_handle = 0;
    small_size = 0;
    small_address = 0;
    small_address_value = 0x22222222;
    small_result = 0x11111111;
    small_calls = 0;
    cache_handle = 0;
    cache_record = 0;
    cache_value = 0;
    cache_name = 0;
    cache_size = 0;
    cache_result = (void *)0x0badf00d;
    cache_calls = 0;
}

int main(void)
{
    void *result;

    reset(2, 2);
    result = resource_get(resource, name, 0x1234);
    put(result == 0 && *(int *)(resource + 0xc) == 2
        && sprintf_calls == 1 && sprintf_buffer == (char *)resource + 0x10
        && sprintf_name == name && same(sprintf_format, "%.11s")
        && same(sprintf_name, name)
        && dos_calls == 0 && small_calls == 0 && cache_calls == 0 ? '0' : 'x');

    reset(0, 2);
    dos_result = 0x12345678;
    result = resource_get(resource, name, 0x100);
    put(result == cache_result && dos_calls == 1
        && dos_handle == resource && dos_size == 0x100
        && dos_selector == (unsigned int *)(records + 0x10)
        && *(unsigned int *)(records + 0x10) == 0x3333
        && *(int *)(records + 0x18) == 1
        && small_calls == 0
        && cache_calls == 1 && cache_handle == resource
        && cache_record == (char *)records && cache_value == (void *)0x12345678
        && cache_name == name && cache_size == 0x100 ? '1' : 'x');

    reset(0, 2);
    dos_result = 0;
    result = resource_get(resource, name, 0x200);
    put(result == cache_result && dos_calls == 1
        && dos_selector == (unsigned int *)(records + 0x10)
        && small_calls == 1 && small_handle == resource && small_size == 0x200
        && small_address == (unsigned int *)(records + 0x10)
        && *(unsigned int *)(records + 0x10) == 0x22222222
        && *(int *)(records + 0x18) == 0
        && cache_calls == 1 && cache_record == (char *)records
        && cache_value == (void *)0x11111111 && cache_size == 0x200 ? '2' : 'x');

    reset(0, 2);
    dos_result = 0x77777777;
    result = resource_get(resource, name, 0x96000);
    put(result == cache_result && dos_calls == 0
        && small_calls == 1 && small_size == 0x96000
        && small_address == (unsigned int *)(records + 0x10)
        && *(unsigned int *)(records + 0x10) == 0x22222222
        && *(int *)(records + 0x18) == 0
        && cache_calls == 1 && cache_record == (char *)records
        && cache_value == (void *)0x11111111 && cache_size == 0x96000 ? '3' : 'x');

    reset(1, 3);
    dos_result = 0x44444444;
    result = resource_get(resource, name, 0x40);
    put(result == cache_result && dos_calls == 1
        && dos_selector == (unsigned int *)(records + 0x1c + 0x10)
        && *(int *)(records + 0x1c + 0x18) == 1
        && small_calls == 0 && cache_calls == 1
        && cache_record == (char *)(records + 0x1c)
        && cache_value == (void *)0x44444444 && cache_size == 0x40 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
