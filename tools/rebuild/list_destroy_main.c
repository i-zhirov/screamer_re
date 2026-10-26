/* The test harness for the rebuilt `list_destroy` module.
 *
 * The harness holds the `free` stub.  The stub counts the calls, and the program calls
 * the module twice: once with a set pointer and once with a null pointer.  It then prints
 * the markers:
 *
 *     F  the free call happened for the set pointer
 *     R  the return value is the first argument
 *     z  the pointer itself stays set, so the module does not clear it
 *     N  the free call did not happen for the null pointer
 *
 * The markers go through the data buffer `text`, because the `print` helper of this harness
 * writes garbage for a string literal (the same fault shows in the output of the driver at
 * work/rebuild/out.txt).
 */
#pragma off (check_stack)

void *list_destroy(void **list, int unused);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char buffer[16];
static char text[8];
static void *block;
static int freed;

void free(void *p)
{
    freed = freed + 1;
}

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int main(void)
{
    void *result;

    block = buffer;
    freed = 0;
    result = list_destroy(&block, 0);
    put(freed == 1 ? 'F' : 'f');
    put(result == (void *)&block ? 'R' : 'r');
    put(block == 0 ? 'z' : 'Z');

    block = 0;
    freed = 0;
    result = list_destroy(&block, 0);
    put(freed == 0 ? 'N' : 'n');
    quit();
    return 0;
}
