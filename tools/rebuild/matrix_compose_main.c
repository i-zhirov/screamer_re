/* The test harness for the rebuilt `matrix_compose` module.
 *
 * The harness holds the source matrix, the affine matrix, and the result
 * matrix as 12-dword arrays.  The source and the result use four columns of
 * three rows, and the matrix uses four rows of three columns.
 *
 * The harness runs five paths:
 *
 *     0  identity: the diagonal 0x4000 and the clear translation row copy
 *        every source value to the result
 *     1  scale and translation: the diagonal 0x8000 doubles the rotation and
 *        the translation row 0x4000 adds the three source rotation values to
 *        the source translation value
 *     2  permutation: the three rotation rows select the b, c, and a columns,
 *        so the result reorders the source groups
 *     3  negative values: the diagonal 0x4000 keeps the signed source values
 *     4  arithmetic shift: the diagonal 0x2000 halves the source values and
 *        rounds each negative product toward negative infinity
 *     D  the exit marker
 *
 * The expected arrays live in the stack frame and take their values at run
 * time.  The minimal image does not load a constant array reliably, so the
 * harness avoids an initialized array here.
 */
#pragma off (check_stack)

void matrix_compose(int *source, int *matrix, int *destination);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static int source[12];
static int matrix[12];
static int destination[12];
static char text[8];

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

    for (index = 0; index < 12; index++) {
        source[index] = 0;
        matrix[index] = 0;
        destination[index] = 0;
    }
}

static void set_source(int first)
{
    int index;

    for (index = 0; index < 12; index++) {
        source[index] = first + index;
    }
}

static void set_negative(void)
{
    int index;

    for (index = 0; index < 12; index++) {
        source[index] = -1 - index;
    }
}

static void set_diagonal(int value)
{
    matrix[0] = value;
    matrix[4] = value;
    matrix[8] = value;
}

static int identity_ok(void)
{
    int index;

    for (index = 0; index < 12; index++) {
        if (destination[index] != index + 1) {
            return 0;
        }
    }
    return 1;
}

static int compare(const int *expected)
{
    int index;

    for (index = 0; index < 12; index++) {
        if (destination[index] != expected[index]) {
            return 0;
        }
    }
    return 1;
}

static int scaled_ok(void)
{
    int expected[12];

    expected[0] = 2;
    expected[1] = 4;
    expected[2] = 6;
    expected[3] = 8;
    expected[4] = 10;
    expected[5] = 12;
    expected[6] = 14;
    expected[7] = 16;
    expected[8] = 18;
    expected[9] = 112;
    expected[10] = 215;
    expected[11] = 318;
    return compare(expected);
}

static int permuted_ok(void)
{
    int expected[12];

    expected[0] = 4;
    expected[1] = 5;
    expected[2] = 6;
    expected[3] = 7;
    expected[4] = 8;
    expected[5] = 9;
    expected[6] = 1;
    expected[7] = 2;
    expected[8] = 3;
    expected[9] = 10;
    expected[10] = 11;
    expected[11] = 12;
    return compare(expected);
}

static int negative_ok(void)
{
    int index;

    for (index = 0; index < 12; index++) {
        if (destination[index] != -1 - index) {
            return 0;
        }
    }
    return 1;
}

static int shifted_ok(void)
{
    int expected[12];

    expected[0] = -2;
    expected[1] = -1;
    expected[2] = -1;
    expected[3] = -2;
    expected[4] = -1;
    expected[5] = -1;
    expected[6] = -3;
    expected[7] = -5;
    expected[8] = -2;
    expected[9] = 0;
    expected[10] = 0;
    expected[11] = 0;
    return compare(expected);
}

int main(void)
{
    reset();
    set_source(1);
    set_diagonal(0x4000);
    matrix_compose(source, matrix, destination);
    put(identity_ok() ? '0' : 'x');

    reset();
    set_source(1);
    source[9] = 100;
    source[10] = 200;
    source[11] = 300;
    set_diagonal(0x8000);
    matrix[9] = 0x4000;
    matrix[10] = 0x4000;
    matrix[11] = 0x4000;
    matrix_compose(source, matrix, destination);
    put(scaled_ok() ? '1' : 'x');

    reset();
    set_source(1);
    matrix[1] = 0x4000;
    matrix[5] = 0x4000;
    matrix[6] = 0x4000;
    matrix_compose(source, matrix, destination);
    put(permuted_ok() ? '2' : 'x');

    reset();
    set_negative();
    set_diagonal(0x4000);
    matrix_compose(source, matrix, destination);
    put(negative_ok() ? '3' : 'x');

    reset();
    source[0] = -3;
    source[1] = -2;
    source[2] = -1;
    source[3] = -4;
    source[4] = -1;
    source[5] = -2;
    source[6] = -5;
    source[7] = -9;
    source[8] = -3;
    set_diagonal(0x2000);
    matrix_compose(source, matrix, destination);
    put(shifted_ok() ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
