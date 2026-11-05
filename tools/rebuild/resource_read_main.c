/* The test harness for the rebuilt `resource_read` module.
 *
 * The harness holds the resource structure, the record array, and the three
 * stubs.  The stubs record their arguments.  The allocation stub writes the
 * configured address to the record cell and returns the configured value.  The
 * cache stub returns the configured value.  The harness runs four paths:
 *
 *     0  the full-table path sets the status to 2, copies the name to
 *        `resource + 0x10`, and returns 0 without an allocation or a cache
 *     1  the first-record path allocates the size at `record + 0x10`, clears
 *        `+0x18`, and caches the record with the name and the size
 *     2  the second-record path uses the 0x1c-byte stride and the same calls
 *     3  the allocation-failure path still clears `+0x18` and caches the zero
 *        result
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

void *resource_read(void *resource, char *name, int size);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char resource[0x20];
static unsigned char records[2 * 0x1c];
static char name[12];
static void *got_name;
static void *sprintf_buffer;
static char *sprintf_format;
static char *sprintf_name;
static void *alloc_handle;
static int alloc_size;
static unsigned int *alloc_address_ptr;
static void *alloc_result;
static unsigned int alloc_address;
static void *cache_handle;
static char *cache_record;
static void *cache_value;
static char *cache_name;
static int cache_size;
static void *cache_result;
static int sprintf_calls;
static int alloc_calls;
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

void *memory_alloc_small(void *handle, int size, unsigned int *address)
{
    alloc_handle = handle;
    alloc_size = size;
    alloc_address_ptr = address;
    *address = alloc_address;
    alloc_calls = alloc_calls + 1;
    return alloc_result;
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
    got_name = 0;
    sprintf_buffer = 0;
    sprintf_format = 0;
    sprintf_name = 0;
    alloc_handle = 0;
    alloc_size = 0;
    alloc_address_ptr = 0;
    alloc_result = (void *)0x9abcdef0;
    alloc_address = 0x12345678;
    cache_handle = 0;
    cache_record = 0;
    cache_value = 0;
    cache_name = 0;
    cache_size = 0;
    cache_result = (void *)0x0badf00d;
    sprintf_calls = 0;
    alloc_calls = 0;
    cache_calls = 0;
}

int main(void)
{
    void *result;

    reset(2, 2);
    result = resource_read(resource, name, 0x1234);
    put(result == 0 && *(int *)(resource + 0xc) == 2
        && sprintf_calls == 1 && sprintf_buffer == (char *)resource + 0x10
        && sprintf_name == name && same(sprintf_name, name)
        && alloc_calls == 0 && cache_calls == 0 ? '0' : 'x');

    reset(0, 2);
    records[0x18] = 0xff;
    result = resource_read(resource, name, 0x40740);
    put(result == cache_result && alloc_calls == 1
        && alloc_handle == resource && alloc_size == 0x40740
        && alloc_address_ptr == (unsigned int *)(records + 0x10)
        && *(unsigned int *)(records + 0x10) == 0x12345678
        && *(int *)(records + 0x18) == 0
        && cache_calls == 1 && cache_handle == resource
        && cache_record == (char *)records && cache_value == alloc_result
        && cache_name == name && cache_size == 0x40740 ? '1' : 'x');

    reset(1, 3);
    alloc_result = (void *)0x11111111;
    alloc_address = 0x22222222;
    result = resource_read(resource, name, 0x100);
    put(result == cache_result && alloc_calls == 1
        && alloc_address_ptr == (unsigned int *)(records + 0x1c + 0x10)
        && *(unsigned int *)(records + 0x1c + 0x10) == 0x22222222
        && cache_record == (char *)(records + 0x1c)
        && cache_value == alloc_result && cache_size == 0x100 ? '2' : 'x');

    reset(0, 2);
    alloc_result = 0;
    alloc_address = 0;
    records[0x18] = 0xff;
    result = resource_read(resource, name, 0x10);
    put(result == cache_result && alloc_calls == 1 && cache_calls == 1
        && *(unsigned int *)(records + 0x10) == 0
        && *(int *)(records + 0x18) == 0 && cache_value == 0 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
