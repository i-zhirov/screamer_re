/* The test harness for the rebuilt `resource_free` module.
 *
 * The harness holds the record table and the two free stubs.  The stubs record
 * their arguments and return a configured result.  The harness runs the two
 * free paths and the two failure paths:
 *
 *     0  the clear flag calls `memory_free` with the first record address
 *     1  the set flag calls `dos_memory_free` with the second record address
 *     2  a non-zero `memory_free` result returns -1
 *     3  a non-zero `dos_memory_free` result returns -1
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int resource_free(void **resource, int index);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char records[2 * 0x1c];
static void *base;
static void *resource;
static int memory_result;
static int dos_result;
static int memory_calls;
static int dos_calls;
static void *got_handle;
static unsigned int got_address;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int memory_free(int handle, unsigned int address)
{
    got_handle = (void *)handle;
    got_address = address;
    memory_calls = memory_calls + 1;
    return memory_result;
}

int dos_memory_free(int handle, unsigned int address)
{
    got_handle = (void *)handle;
    got_address = address;
    dos_calls = dos_calls + 1;
    return dos_result;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 2 * 0x1c; ++index) {
        records[index] = 0;
    }
    *(unsigned int *)(records + 0x10) = 0x11111111;
    records[0x18] = 0;
    *(unsigned int *)(records + 0x1c + 0x10) = 0x22222222;
    records[0x1c + 0x18] = 1;
    base = records;
    resource = &base;
    memory_result = 0;
    dos_result = 0;
    memory_calls = 0;
    dos_calls = 0;
    got_handle = 0;
    got_address = 0;
}

int main(void)
{
    int result;

    reset();
    result = resource_free(&base, 0);
    put(result == 0 && memory_calls == 1 && dos_calls == 0
        && got_handle == resource && got_address == 0x11111111 ? '0' : 'x');

    reset();
    result = resource_free(&base, 1);
    put(result == 0 && memory_calls == 0 && dos_calls == 1
        && got_handle == resource && got_address == 0x22222222 ? '1' : 'x');

    reset();
    memory_result = 1;
    result = resource_free(&base, 0);
    put(result == -1 && memory_calls == 1 && dos_calls == 0 ? '2' : 'x');

    reset();
    dos_result = 1;
    result = resource_free(&base, 1);
    put(result == -1 && memory_calls == 0 && dos_calls == 1 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
