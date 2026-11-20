/* The test harness for the rebuilt `camera_basis_patch` module.
 *
 * The harness holds the saved camera position and a 3x3 basis.  It calls the
 * module with the basis pointer, then reads the two result globals.
 *
 * The paths:
 *
 *     0  the basis 1 through 9 and the position 10 through 12
 *     1  the basis 100 through 108 with the negative position -1 through 1
 *     2  an all-zero basis and an all-zero position
 *     3  two calls: the second call overwrites every slot of the first call
 *     4  the descending basis -1 through -9 keeps the high-bit values
 *     D  the exit marker
 *
 * The expected basis slot `d` holds the source slot `(d % 3) * 3 + d / 3`,
 * because the module reads the basis in the column order.  The harness forms
 * the expected value from the source array, so it needs no constant data
 * array.  Every expected value is a plain integer, so the harness needs no
 * float literal and no constant segment.
 */
#pragma off (check_stack)

void camera_basis_patch(int *basis);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int saved_position[3];
extern int mix_prepare_basis[9];
extern int mix_prepare_position[3];

static char text[8];
static int basis[9];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static void set_basis(int first)
{
    int index;

    for (index = 0; index < 9; index++) {
        basis[index] = first + index;
    }
}

static void set_basis_descending(int first)
{
    int index;

    for (index = 0; index < 9; index++) {
        basis[index] = first - index;
    }
}

static void set_position(int first)
{
    int index;

    for (index = 0; index < 3; index++) {
        saved_position[index] = first + index;
    }
}

static int basis_ok(void)
{
    int slot;
    int source;

    for (slot = 0; slot < 9; slot++) {
        source = (slot % 3) * 3 + slot / 3;
        if (mix_prepare_basis[slot] != basis[source]) {
            return 0;
        }
    }
    return 1;
}

static int position_ok(int first)
{
    int index;

    for (index = 0; index < 3; index++) {
        if (mix_prepare_position[index] != first + index) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    set_basis(1);
    set_position(10);
    camera_basis_patch(basis);
    check('0', basis_ok() && position_ok(10));

    set_basis(100);
    set_position(-1);
    camera_basis_patch(basis);
    check('1', basis_ok() && position_ok(-1));

    set_basis(0);
    set_position(0);
    camera_basis_patch(basis);
    check('2', basis_ok() && position_ok(0));

    set_basis(1);
    set_position(10);
    camera_basis_patch(basis);
    set_basis(11);
    set_position(30);
    camera_basis_patch(basis);
    check('3', basis_ok() && position_ok(30));

    set_basis_descending(-1);
    set_position(-1);
    camera_basis_patch(basis);
    check('4', basis_ok() && position_ok(-1));

    put('D');
    quit();
    return 0;
}
