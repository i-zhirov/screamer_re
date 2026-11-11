/* The test harness for the rebuilt `basis_vector_transform` module.
 *
 * The harness holds one 12-dword record: the nine Q14 matrix values at
 * `+0x00` through `+0x20` and the three result dwords at `+0x24` through
 * `+0x2c`.
 *
 * The harness runs four paths:
 *
 *     0  identity: the diagonal 0x4000 copies the vector to the result
 *     1  scale and cross terms: three distinct rows multiply the vector
 *     2  negative values: the diagonal 0x2000 halves the vector and the
 *        arithmetic shift rounds each negative product toward negative
 *        infinity
 *     3  mixed signs: the negative matrix elements and the negative vector
 *        element reach the three results
 *     D  the exit marker
 *
 * The expected values live in the stack frame and take their values at run
 * time.  The minimal image does not load a constant array reliably.
 */
#pragma off (check_stack)

void basis_vector_transform(int *matrix, int x, int y, int z);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static int record[12];
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

static void clear(void)
{
    int index;

    for (index = 0; index < 12; index++) {
        record[index] = 0;
    }
}

static void set_diagonal(int value)
{
    record[0] = value;
    record[4] = value;
    record[8] = value;
}

static int result_is(int first, int second, int third)
{
    return record[9] == first
        && record[10] == second
        && record[11] == third;
}

int main(void)
{
    clear();
    set_diagonal(0x4000);
    basis_vector_transform(record, 100, -200, 300);
    put(result_is(100, -200, 300) ? '0' : 'x');

    clear();
    record[0] = 0x2000;
    record[3] = 0x1000;
    record[6] = 0x0800;
    record[1] = 0x1000;
    record[4] = 0x2000;
    record[7] = 0x1000;
    record[2] = 0x0800;
    record[5] = 0x1000;
    record[8] = 0x2000;
    basis_vector_transform(record, 2, 3, 4);
    put(result_is(2, 3, 3) ? '1' : 'x');

    clear();
    set_diagonal(0x2000);
    basis_vector_transform(record, -3, -2, -1);
    put(result_is(-2, -1, -1) ? '2' : 'x');

    clear();
    record[0] = 0x1000;
    record[3] = -0x2000;
    record[6] = 0x0800;
    record[1] = -0x1000;
    record[4] = 0x2000;
    record[7] = 0x1000;
    record[2] = 0x2000;
    record[5] = 0x1000;
    record[8] = -0x2000;
    basis_vector_transform(record, 5, -7, 11);
    put(result_is(6, -2, -5) ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
