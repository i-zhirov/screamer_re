/* The test harness for the rebuilt `basis_rotation_mirror` module.
 *
 * The harness holds one 9-dword matrix and one 9-dword expected block.  It
 * resets the matrix before each call and compares all nine dwords after the
 * call.
 *
 * The five paths are:
 *
 *     0  the unity values: cos = 0x4000 and sin = 0
 *     1  mixed positive and negative values: the generic matrix
 *     2  small values: the arithmetic shift rounds the negative sums down
 *     3  the table limits: the negatives and the sign changes
 *     4  the large values: the products reach the 32-bit range
 *     D  the exit marker
 *
 * The expected values come from the original arithmetic in this file.  The
 * harness does not use initialized static data, because the minimal image
 * leaves the initialized data pages zero under DOS/4G.
 */
#pragma off (check_stack)

void basis_rotation_mirror(int *matrix, int cos0, int sin0,
    int cos1, int sin1, int cos2, int sin2);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

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

static int array_equal(const int *left, const int *right)
{
    int index;

    for (index = 0; index < 9; index++) {
        if (left[index] != right[index]) {
            return 0;
        }
    }
    return 1;
}

static void clear_matrix(int *matrix)
{
    int index;

    for (index = 0; index < 9; index++) {
        matrix[index] = 0x11111111;
    }
}

int main(void)
{
    int matrix[9];
    int expected[9];

    clear_matrix(matrix);
    expected[0] = 16384;
    expected[1] = 0;
    expected[2] = 0;
    expected[3] = 0;
    expected[4] = 0;
    expected[5] = 16384;
    expected[6] = 0;
    expected[7] = -16384;
    expected[8] = 0;
    basis_rotation_mirror(matrix, 0x4000, 0, 0x4000, 0, 0x4000, 0);
    put(array_equal(matrix, expected) ? '0' : 'x');

    clear_matrix(matrix);
    expected[0] = -13925;
    expected[1] = -78;
    expected[2] = 4394;
    expected[3] = 3418;
    expected[4] = 6835;
    expected[5] = 12000;
    expected[6] = -3623;
    expected[7] = 11981;
    expected[8] = -6348;
    basis_rotation_mirror(matrix, 12000, 8000, 9000, -13000, -7000, 14000);
    put(array_equal(matrix, expected) ? '1' : 'x');

    clear_matrix(matrix);
    expected[0] = 0;
    expected[1] = -1;
    expected[2] = 0;
    expected[3] = 0;
    expected[4] = -1;
    expected[5] = 100;
    expected[6] = 0;
    expected[7] = -1;
    expected[8] = -1;
    basis_rotation_mirror(matrix, 100, -3, -7, 200, -11, 13);
    put(array_equal(matrix, expected) ? '2' : 'x');

    clear_matrix(matrix);
    expected[0] = 0;
    expected[1] = 16384;
    expected[2] = 0;
    expected[3] = 0;
    expected[4] = 0;
    expected[5] = -16384;
    expected[6] = -16384;
    expected[7] = 0;
    expected[8] = 0;
    basis_rotation_mirror(matrix, -16384, 0, 0, 16384, 16384, 0);
    put(array_equal(matrix, expected) ? '3' : 'x');

    clear_matrix(matrix);
    expected[0] = -16383;
    expected[1] = 0;
    expected[2] = -1;
    expected[3] = 0;
    expected[4] = -1;
    expected[5] = 16383;
    expected[6] = -1;
    expected[7] = 16382;
    expected[8] = 0;
    basis_rotation_mirror(matrix, 16383, 1, -16383, 1, 16383, -1);
    put(array_equal(matrix, expected) ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
