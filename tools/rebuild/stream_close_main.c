/* The test harness for the rebuilt `stream_close` module.
 *
 * The harness holds a stream record and the three stubs.  The stubs record
 * their arguments.  The harness calls the module with the two buffers set and
 * then with the two guards cleared:
 *
 *     0  the first call released both buffers and closed the handle
 *     1  the second call released no buffer and closed the handle again
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int stream_close(char *stream);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char record[0xc0];
static char buffer_a;
static char buffer_b;
static void *nfree_pointer;
static void *free_pointer;
static int close_handle;
static int nfree_calls;
static int free_calls;
static int close_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void nfree(void *p)
{
    nfree_pointer = p;
    nfree_calls = nfree_calls + 1;
}

void free(void *p)
{
    free_pointer = p;
    free_calls = free_calls + 1;
}

int handle_close(int handle)
{
    close_handle = handle;
    close_calls = close_calls + 1;
    return 0;
}

int main(void)
{
    int index;

    for (index = 0; index < 0xc0; ++index) {
        record[index] = 0;
    }
    *(void **)(record + 0x1c) = &buffer_a;
    *(void **)(record + 0xb0) = &buffer_b;
    *(int *)(record + 0x10) = 1;
    *(int *)record = 7;
    stream_close((char *)record);
    put(nfree_calls == 1 && nfree_pointer == &buffer_a
        && free_calls == 1 && free_pointer == &buffer_b
        && close_calls == 1 && close_handle == 7 ? '0' : 'x');

    nfree_calls = 0;
    free_calls = 0;
    close_calls = 0;
    *(void **)(record + 0x1c) = 0;
    *(int *)(record + 0x10) = 0;
    *(int *)record = 9;
    stream_close((char *)record);
    put(nfree_calls == 0 && free_calls == 0
        && close_calls == 1 && close_handle == 9 ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
