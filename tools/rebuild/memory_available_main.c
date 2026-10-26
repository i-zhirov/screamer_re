/* The test harness for the rebuilt `memory_available` module.
 *
 * The harness holds the two probe stubs.  Each stub counts its calls and returns a fixed
 * value, and the program checks the sum, so the output shows the chain:
 *
 *     F  the first probe ran once
 *     S  the second probe ran once
 *     T  the total is the sum of the two
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of this harness
 * writes garbage for a string literal (the same fault shows in the output of the driver at
 * work/rebuild/out.txt).
 */
#pragma off (check_stack)

unsigned memory_available(void *pool);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static int free_calls;
static int size_calls;
static int marker;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

unsigned probe_free(void *pool)
{
    free_calls = free_calls + 1;
    return 3;
}

unsigned probe_size(void *pool)
{
    size_calls = size_calls + 1;
    return 4;
}

int main(void)
{
    unsigned total;

    free_calls = 0;
    size_calls = 0;
    total = memory_available(&marker);
    put(free_calls == 1 ? 'F' : 'f');
    put(size_calls == 1 ? 'S' : 's');
    put(total == 7 ? 'T' : 't');
    put('D');
    quit();
    return 0;
}
