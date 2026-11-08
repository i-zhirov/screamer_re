/* The test harness for the rebuilt `resource_find` module.
 *
 * The harness holds the resource record, the four 0x1c-byte records, and the
 * two stubs.  The free stub records its arguments and returns a configured
 * result.  The memmove stub records its arguments and copies the bytes.  The
 * harness runs four paths:
 *
 *     0  the key at index 0 frees that record, shifts the array, decrements the
 *        count, sets the status to 0, and returns 0
 *     1  the key at index 1 uses the record stride and shifts the array
 *     2  a missing key returns -1, sets the status to 3, and calls no stub
 *     3  a non-zero `resource_free` result returns -1, sets the status to 3,
 *        and does not shift the array
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int resource_find(void *resource, int key);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char resource[0x20];
static unsigned char records[4 * 0x1c];
static void *free_handle;
static int free_index;
static int free_result;
static void *move_destination;
static void *move_source;
static unsigned int move_size;
static int free_calls;
static int move_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int resource_free(void **resource, int index)
{
    free_handle = resource;
    free_index = index;
    free_calls = free_calls + 1;
    return free_result;
}

void *memmove(void *destination, const void *source, unsigned int size)
{
    unsigned char *to;
    const unsigned char *from;
    unsigned int index;

    move_destination = destination;
    move_source = (void *)source;
    move_size = size;
    move_calls = move_calls + 1;
    to = destination;
    from = source;
    for (index = 0; index < size; ++index) {
        to[index] = from[index];
    }
    return destination;
}

static void reset(int count)
{
    int index;

    for (index = 0; index < 0x20; ++index) {
        resource[index] = 0;
    }
    for (index = 0; index < 4 * 0x1c; ++index) {
        records[index] = 0;
    }
    *(void **)resource = records;
    *(int *)(resource + 4) = count;
    *(int *)(resource + 8) = 4;
    *(int *)(records + 0 * 0x1c + 0xc) = 0x100;
    *(int *)(records + 0 * 0x1c + 0x10) = 0xaaaa0000;
    *(int *)(records + 1 * 0x1c + 0xc) = 0x200;
    *(int *)(records + 1 * 0x1c + 0x10) = 0xbbbb1111;
    *(int *)(records + 2 * 0x1c + 0xc) = 0x300;
    *(int *)(records + 2 * 0x1c + 0x10) = 0xcccc2222;
    *(int *)(records + 3 * 0x1c + 0xc) = 0x400;
    *(int *)(records + 3 * 0x1c + 0x10) = 0xdddd3333;
    free_handle = 0;
    free_index = 0;
    free_result = 0;
    move_destination = 0;
    move_source = 0;
    move_size = 0;
    free_calls = 0;
    move_calls = 0;
}

int main(void)
{
    int result;

    reset(3);
    result = resource_find(resource, 0x100);
    put(result == 0 && *(int *)(resource + 0xc) == 0
        && *(int *)(resource + 4) == 2
        && free_calls == 1 && free_handle == resource && free_index == 0
        && move_calls == 1 && move_destination == records
        && move_source == records + 0x1c && move_size == 3 * 0x1c
        && *(int *)(records + 0x10) == 0xbbbb1111
        && *(int *)(records + 0x1c + 0xc) == 0x300 ? '0' : 'x');

    reset(3);
    result = resource_find(resource, 0x200);
    put(result == 0 && *(int *)(resource + 0xc) == 0
        && *(int *)(resource + 4) == 2
        && free_calls == 1 && free_handle == resource && free_index == 1
        && move_calls == 1 && move_destination == records + 0x1c
        && move_source == records + 2 * 0x1c && move_size == 2 * 0x1c
        && *(int *)(records + 0x1c + 0x10) == 0xcccc2222
        && *(int *)(records + 0x1c + 0xc) == 0x300 ? '1' : 'x');

    reset(2);
    result = resource_find(resource, 0x999);
    put(result == -1 && *(int *)(resource + 0xc) == 3
        && *(int *)(resource + 4) == 2 && free_calls == 0
        && move_calls == 0 ? '2' : 'x');

    reset(2);
    free_result = 1;
    result = resource_find(resource, 0x100);
    put(result == -1 && *(int *)(resource + 0xc) == 3
        && *(int *)(resource + 4) == 2 && free_calls == 1 && free_index == 0
        && move_calls == 0
        && *(int *)(records + 0x10) == 0xaaaa0000 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
