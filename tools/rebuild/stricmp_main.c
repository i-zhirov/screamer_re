/* The test harness for the rebuilt `stricmp` module.
 *
 * The harness holds two data buffers and fills them with explicit byte values,
 * because the minimal image does not load a constant data segment.  Each path
 * writes the two strings, calls `stricmp`, and compares the result with the
 * expected difference.
 *
 * The harness runs nine paths:
 *
 *     0  the equal strings "abc" and "abc" return 0
 *     1  the case-insensitive strings "Hello" and "hELLo" return 0
 *     2  "ABC" and "abd" return -1, because the lowercase bytes are 'c' and 'd'
 *     3  "abd" and "ABC" return 1
 *     4  "a" and the empty string return 0x61
 *     5  the empty string and "a" return -0x61
 *     6  "abc" and "abcd" return -0x64 at the zero byte
 *     7  "[" and "{" stay unchanged and return -0x20
 *     8  "Zebra" and "zEBRA" return 0 at the end of both strings
 *     D  the exit marker
 *
 * Every path checks the exact result.
 */
#pragma off (check_stack)

int stricmp(const char *first, const char *second);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char first[8];
static char second[8];
static char text[8];

static void fill(char *target, char a, char b, char c, char d, char e)
{
    target[0] = a;
    target[1] = b;
    target[2] = c;
    target[3] = d;
    target[4] = e;
    target[5] = 0;
}

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

static void compare(int expected, char marker)
{
    report(stricmp(first, second) == expected, marker);
}

int main(void)
{
    fill(first, 'a', 'b', 'c', 0, 0);
    fill(second, 'a', 'b', 'c', 0, 0);
    compare(0, '0');

    fill(first, 'H', 'e', 'l', 'l', 'o');
    fill(second, 'h', 'E', 'L', 'L', 'o');
    compare(0, '1');

    fill(first, 'A', 'B', 'C', 0, 0);
    fill(second, 'a', 'b', 'd', 0, 0);
    compare(-1, '2');

    fill(first, 'a', 'b', 'd', 0, 0);
    fill(second, 'A', 'B', 'C', 0, 0);
    compare(1, '3');

    fill(first, 'a', 0, 0, 0, 0);
    fill(second, 0, 0, 0, 0, 0);
    compare(0x61, '4');

    fill(first, 0, 0, 0, 0, 0);
    fill(second, 'a', 0, 0, 0, 0);
    compare(-0x61, '5');

    fill(first, 'a', 'b', 'c', 0, 0);
    fill(second, 'a', 'b', 'c', 'd', 0);
    compare(-0x64, '6');

    fill(first, '[', 0, 0, 0, 0);
    fill(second, '{', 0, 0, 0, 0);
    compare(-0x20, '7');

    fill(first, 'Z', 'e', 'b', 'r', 'a');
    fill(second, 'z', 'E', 'B', 'R', 'A');
    compare(0, '8');

    put('D');
    quit();
    return 0;
}
