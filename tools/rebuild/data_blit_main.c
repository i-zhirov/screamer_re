/* The test harness for the rebuilt `data_blit` module.
 *
 * The harness holds one compressed source stream and one target block per path.
 * The target and the expected block start with the fill byte 0xa5, so a skipped
 * span is visible as an unchanged range.
 *
 * The five paths:
 *
 *     0  one positive span of six bytes
 *     1  two positive spans around a negative three-byte skip
 *     2  a dword span of four bytes and a remaining span of three bytes
 *     3  a zero span at once, so the target stays unchanged
 *     4  three one-byte spans
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness does not reach a string literal.
 */
#pragma off (check_stack)

void data_blit(void *source, void *destination);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char source[64];
static unsigned char target[64];
static unsigned char expect[64];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(void)
{
    int index;
    for (index = 0; index < 64; index++) {
        source[index] = 0;
        target[index] = 0xa5;
        expect[index] = 0xa5;
    }
}

static int same(void)
{
    int index;
    for (index = 0; index < 64; index++) {
        if (target[index] != expect[index]) {
            return 0;
        }
    }
    return 1;
}

static void put_length(int offset, int value)
{
    *(int *)(source + offset) = value;
}

int main(void)
{
    reset();
    put_length(0, 6);
    source[4] = 'a';
    source[5] = 'b';
    source[6] = 'c';
    source[7] = 'd';
    source[8] = 'e';
    source[9] = 'f';
    put_length(10, 0);
    expect[0] = 'a';
    expect[1] = 'b';
    expect[2] = 'c';
    expect[3] = 'd';
    expect[4] = 'e';
    expect[5] = 'f';
    data_blit(source, target);
    put(same() ? '0' : 'x');

    reset();
    put_length(0, 2);
    source[4] = 'h';
    source[5] = 'i';
    put_length(6, -3);
    put_length(10, 2);
    source[14] = 'y';
    source[15] = 'o';
    put_length(16, 0);
    expect[0] = 'h';
    expect[1] = 'i';
    expect[5] = 'y';
    expect[6] = 'o';
    data_blit(source, target);
    put(same() ? '1' : 'x');

    reset();
    put_length(0, 4);
    source[4] = 'W';
    source[5] = 'X';
    source[6] = 'Y';
    source[7] = 'Z';
    put_length(8, 3);
    source[12] = '1';
    source[13] = '2';
    source[14] = '3';
    put_length(15, 0);
    expect[0] = 'W';
    expect[1] = 'X';
    expect[2] = 'Y';
    expect[3] = 'Z';
    expect[4] = '1';
    expect[5] = '2';
    expect[6] = '3';
    data_blit(source, target);
    put(same() ? '2' : 'x');

    reset();
    put_length(0, 0);
    data_blit(source, target);
    put(same() ? '3' : 'x');

    reset();
    put_length(0, 1);
    source[4] = 'A';
    put_length(5, 1);
    source[9] = 'B';
    put_length(10, 1);
    source[14] = 'C';
    put_length(15, 0);
    expect[0] = 'A';
    expect[1] = 'B';
    expect[2] = 'C';
    data_blit(source, target);
    put(same() ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
