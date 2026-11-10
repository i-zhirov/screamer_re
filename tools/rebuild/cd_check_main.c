/* The test harness for the rebuilt `cd_check` module.
 *
 * The harness holds the CD driver response buffer, the four text cells, and the
 * two callee stubs.  The `sub_26014` stub returns the response buffer or a null
 * pointer.  The `strnicmp` stub keeps the real case-insensitive,
 * length-limited behaviour, and it counts its calls.
 *
 * The four text cells are mutable global arrays, and the harness fills them at
 * run time, because the initialized data pages of this minimal image stay zero.
 *
 * The harness runs eight paths:
 *
 *     0  the driver returns a null pointer: the result 1 and no compare
 *     1  every field matches: the result 0 and four compares
 *     2  the first field is wrong: the result 1 and one compare
 *     3  the second field is wrong: the result 1 and two compares
 *     4  the third field is wrong: the result 1 and three compares
 *     5  the fourth field is wrong: the result 1 and four compares
 *     6  every field is lower case: the result 0 and four compares
 *     7  every field has a longer correct prefix: the count limits the compare,
 *        so the result is 0 and the four compares run
 *     D  the exit marker
 */
#pragma off (check_stack)

int cd_check(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_0010067e[3];
char DAT_00100681[9];
char DAT_0010068a[4];
char DAT_0010068e[9];

#define RESPONSE_SIZE 0x280

static char response[RESPONSE_SIZE];
static char text[8];
static int driver_present;
static int compare_calls;

void *sub_26014(void)
{
    return driver_present ? response : 0;
}

int strnicmp(const char *left, const char *right, int count)
{
    int index;
    unsigned char a;
    unsigned char b;

    compare_calls = compare_calls + 1;
    for (index = 0; index < count; ++index) {
        a = (unsigned char)left[index];
        b = (unsigned char)right[index];
        if (a >= 'A' && a <= 'Z')
            a = (unsigned char)(a + 0x20);
        if (b >= 'A' && b <= 'Z')
            b = (unsigned char)(b + 0x20);
        if (a != b)
            return (int)a - (int)b;
        if (a == 0)
            return 0;
    }
    return 0;
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

static void copy(char *destination, char *source)
{
    int index;

    for (index = 0; source[index] != 0; ++index) {
        destination[index] = source[index];
    }
    destination[index] = 0;
}

static void lower(char *value)
{
    int index;

    for (index = 0; value[index] != 0; ++index) {
        if (value[index] >= 'A' && value[index] <= 'Z')
            value[index] = (char)(value[index] + 0x20);
    }
}

static void reset(void)
{
    int index;

    for (index = 0; index < RESPONSE_SIZE; ++index)
        response[index] = 0;
    driver_present = 1;
    compare_calls = 0;
}

static void fill_correct(void)
{
    copy(response + 8, DAT_0010067e);
    copy(response + 0x28, DAT_00100681);
    copy(response + 0x13e, DAT_0010068a);
    copy(response + 0x23e, DAT_0010068e);
}

static void init_texts(void)
{
    DAT_0010067e[0] = 'P';
    DAT_0010067e[1] = 'C';
    DAT_0010067e[2] = 0;

    DAT_00100681[0] = 'S';
    DAT_00100681[1] = 'C';
    DAT_00100681[2] = 'R';
    DAT_00100681[3] = 'E';
    DAT_00100681[4] = 'A';
    DAT_00100681[5] = 'M';
    DAT_00100681[6] = 'E';
    DAT_00100681[7] = 'R';
    DAT_00100681[8] = 0;

    DAT_0010068a[0] = 'V';
    DAT_0010068a[1] = 'I';
    DAT_0010068a[2] = 'E';
    DAT_0010068a[3] = 0;

    DAT_0010068e[0] = 'S';
    DAT_0010068e[1] = 'C';
    DAT_0010068e[2] = 'R';
    DAT_0010068e[3] = 'E';
    DAT_0010068e[4] = 'A';
    DAT_0010068e[5] = 'M';
    DAT_0010068e[6] = 'E';
    DAT_0010068e[7] = 'R';
    DAT_0010068e[8] = 0;
}

int main(void)
{
    int result;

    init_texts();

    reset();
    driver_present = 0;
    result = cd_check();
    report(result == 1 && compare_calls == 0, '0');

    reset();
    fill_correct();
    result = cd_check();
    report(result == 0 && compare_calls == 4, '1');

    reset();
    fill_correct();
    response[8] = 'Z';
    result = cd_check();
    report(result == 1 && compare_calls == 1, '2');

    reset();
    fill_correct();
    response[0x28 + 3] = 'X';
    result = cd_check();
    report(result == 1 && compare_calls == 2, '3');

    reset();
    fill_correct();
    response[0x13e + 2] = 'X';
    result = cd_check();
    report(result == 1 && compare_calls == 3, '4');

    reset();
    fill_correct();
    response[0x23e + 7] = 'Q';
    result = cd_check();
    report(result == 1 && compare_calls == 4, '5');

    reset();
    fill_correct();
    lower(response + 8);
    lower(response + 0x28);
    lower(response + 0x13e);
    lower(response + 0x23e);
    result = cd_check();
    report(result == 0 && compare_calls == 4, '6');

    reset();
    fill_correct();
    response[8 + 2] = 'X';
    response[8 + 3] = 0;
    response[0x28 + 8] = 'X';
    response[0x28 + 9] = 0;
    response[0x13e + 3] = 'X';
    response[0x13e + 4] = 0;
    response[0x23e + 8] = 'X';
    response[0x23e + 9] = 0;
    result = cd_check();
    report(result == 0 && compare_calls == 4, '7');

    put('D');
    quit();
    return 0;
}
