/* The test harness for the rebuilt `basis_transform_build` module.
 *
 * The harness holds one 0x78-byte object record, the 0x800-entry angle table,
 * and the three callee stubs.  The angle table entry holds a signed 16-bit
 * cosine at `+0x00` and a signed 16-bit sine at `+0x04`.
 *
 * Each stub records the call order, the matrix pointer, and the six or three
 * arguments.  The harness does not rebuild the rotation of a callee, because
 * the identity of the callees is out of scope.  It verifies the data that the
 * module passes to them and the position that the module saves.
 *
 * The harness runs these paths:
 *
 *     0  three angles with positive, negative, and odd values: the forward
 *        basis takes the Q14 pairs of the three angles, the inverse basis
 *        takes the Q14 pairs of the complement indices 0x7ff - angle, the
 *        saved position equals the position, and the vector transform takes
 *        the negated position
 *     1  the table limits 0x7fff and -0x8000: the arithmetic shift keeps the
 *        sign, and the complement indices reuse the shared table entries
 *     2  two calls on one object: the module reads the object fields again,
 *        so the second call sees the new angle and the new position
 *     D  the exit marker
 */
#pragma off (check_stack)

void basis_transform_build(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define MIRROR_BASE 0x7ff
#define TABLE_SIZE 0x800

struct angle_entry {
    short cosine;
    short reserved_00;
    short sine;
    short reserved_04;
};

struct basis_object {
    int angle0;
    int angle1;
    int angle2;
    int position_x;
    int position_y;
    int position_z;
    int forward[9];
    int saved_x;
    int saved_y;
    int saved_z;
    int inverse[12];
};

struct angle_entry angle_table[TABLE_SIZE];

static struct basis_object object;
static char text[8];

static int sequence;
static int rot_calls;
static int rot_order;
static int *rot_matrix;
static int rot_value[6];
static int mir_calls;
static int mir_order;
static int *mir_matrix;
static int mir_value[6];
static int vec_calls;
static int vec_order;
static int *vec_matrix;
static int vec_value[3];

void basis_rotation_build(int *matrix, int cos0, int sin0,
    int cos1, int sin1, int cos2, int sin2)
{
    rot_calls = rot_calls + 1;
    rot_order = sequence;
    sequence = sequence + 1;
    rot_matrix = matrix;
    rot_value[0] = cos0;
    rot_value[1] = sin0;
    rot_value[2] = cos1;
    rot_value[3] = sin1;
    rot_value[4] = cos2;
    rot_value[5] = sin2;
}

void basis_rotation_mirror(int *matrix, int cos0, int sin0,
    int cos1, int sin1, int cos2, int sin2)
{
    mir_calls = mir_calls + 1;
    mir_order = sequence;
    sequence = sequence + 1;
    mir_matrix = matrix;
    mir_value[0] = cos0;
    mir_value[1] = sin0;
    mir_value[2] = cos1;
    mir_value[3] = sin1;
    mir_value[4] = cos2;
    mir_value[5] = sin2;
}

void basis_vector_transform(int *matrix, int x, int y, int z)
{
    vec_calls = vec_calls + 1;
    vec_order = sequence;
    sequence = sequence + 1;
    vec_matrix = matrix;
    vec_value[0] = x;
    vec_value[1] = y;
    vec_value[2] = z;
}

static void set_angle(int index, int cosine, int sine)
{
    angle_table[index].cosine = (short)cosine;
    angle_table[index].sine = (short)sine;
}

static void clear_table(void)
{
    int index;

    for (index = 0; index < TABLE_SIZE; index++) {
        angle_table[index].cosine = 0;
        angle_table[index].sine = 0;
    }
}

static void reset_record(void)
{
    int index;

    for (index = 0; index < 9; index++) {
        object.forward[index] = 0x11111111;
    }
    for (index = 0; index < 12; index++) {
        object.inverse[index] = 0x22222222;
    }
    object.saved_x = 0x33333333;
    object.saved_y = 0x33333333;
    object.saved_z = 0x33333333;
    rot_calls = 0;
    rot_order = -1;
    rot_matrix = 0;
    mir_calls = 0;
    mir_order = -1;
    mir_matrix = 0;
    vec_calls = 0;
    vec_order = -1;
    vec_matrix = 0;
    sequence = 0;
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

static int rot_are(int c0, int s0, int c1, int s1, int c2, int s2)
{
    return rot_value[0] == c0
        && rot_value[1] == s0
        && rot_value[2] == c1
        && rot_value[3] == s1
        && rot_value[4] == c2
        && rot_value[5] == s2;
}

static int mir_are(int c0, int s0, int c1, int s1, int c2, int s2)
{
    return mir_value[0] == c0
        && mir_value[1] == s0
        && mir_value[2] == c1
        && mir_value[3] == s1
        && mir_value[4] == c2
        && mir_value[5] == s2;
}

static int vec_are(int x, int y, int z)
{
    return vec_value[0] == x
        && vec_value[1] == y
        && vec_value[2] == z;
}

static int calls_are(int rotation, int mirror, int vector)
{
    return rot_calls == rotation
        && mir_calls == mirror
        && vec_calls == vector;
}

int main(void)
{
    int ok;

    clear_table();
    reset_record();
    object.angle0 = 3;
    object.angle1 = 0x20;
    object.angle2 = 0x101;
    object.position_x = 1000;
    object.position_y = -2000;
    object.position_z = 3000;
    set_angle(3, 1000, -2000);
    set_angle(0x20, -4, 7);
    set_angle(0x101, 0x1234, -0x1235);
    set_angle(MIRROR_BASE - 3, -5, 9);
    set_angle(MIRROR_BASE - 0x20, -0x4000, 0x3fff);
    set_angle(MIRROR_BASE - 0x101, 1, -1);
    basis_transform_build(&object);
    ok = calls_are(1, 1, 1)
        && rot_matrix == object.forward
        && mir_matrix == object.inverse
        && vec_matrix == object.inverse
        && rot_order == 0
        && mir_order == 1
        && vec_order == 2
        && rot_are(500, -1000, -2, 3, 2330, -2331)
        && mir_are(-3, 4, -8192, 8191, 0, -1)
        && vec_are(-1000, 2000, -3000)
        && object.saved_x == 1000
        && object.saved_y == -2000
        && object.saved_z == 3000;
    put(ok ? '0' : 'x');

    clear_table();
    reset_record();
    object.angle0 = 0;
    object.angle1 = 0x7ff;
    object.angle2 = 0x400;
    object.position_x = -7;
    object.position_y = 11;
    object.position_z = -13;
    set_angle(0, 0x7fff, -0x8000);
    set_angle(0x7ff, -0x8000, 0x7fff);
    set_angle(0x400, 12345, -12345);
    set_angle(0x3ff, -7, 11);
    basis_transform_build(&object);
    ok = calls_are(1, 1, 1)
        && rot_matrix == object.forward
        && mir_matrix == object.inverse
        && vec_matrix == object.inverse
        && rot_are(16383, -16384, -16384, 16383, 6172, -6173)
        && mir_are(-16384, 16383, 16383, -16384, -4, 5)
        && vec_are(7, -11, 13)
        && object.saved_x == -7
        && object.saved_y == 11
        && object.saved_z == -13;
    put(ok ? '1' : 'x');

    clear_table();
    reset_record();
    object.angle0 = 0x17;
    object.angle1 = 0x18;
    object.angle2 = 0x19;
    object.position_x = 100;
    object.position_y = 200;
    object.position_z = 300;
    set_angle(0x17, 100, 200);
    set_angle(0x18, 300, 400);
    set_angle(0x19, 500, 600);
    set_angle(MIRROR_BASE - 0x17, 700, 800);
    set_angle(MIRROR_BASE - 0x18, 900, 1000);
    set_angle(MIRROR_BASE - 0x19, 1100, 1200);
    basis_transform_build(&object);
    ok = calls_are(1, 1, 1)
        && object.saved_x == 100
        && object.saved_y == 200
        && object.saved_z == 300;

    object.angle0 = 0x1a;
    object.position_x = -100;
    object.position_y = -200;
    object.position_z = -300;
    set_angle(0x1a, -600, 800);
    set_angle(MIRROR_BASE - 0x1a, -900, 1000);
    basis_transform_build(&object);
    ok = ok
        && calls_are(2, 2, 2)
        && rot_matrix == object.forward
        && mir_matrix == object.inverse
        && vec_matrix == object.inverse
        && rot_are(-300, 400, 150, 200, 250, 300)
        && mir_are(-450, 500, 450, 500, 550, 600)
        && vec_are(100, 200, 300)
        && object.saved_x == -100
        && object.saved_y == -200
        && object.saved_z == -300;
    put(ok ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
