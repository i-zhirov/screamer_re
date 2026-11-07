/* The test harness for the rebuilt `quad_point_test` module.
 *
 * The harness holds the four-vertex palette, the polygon entry, the object
 * translation, the test point, the four scratch world points, and the four
 * bounding-box outputs.
 *
 * The edge test accepts a point when it stays on one side of every edge for a
 * polygon that runs clockwise in the y-up plane.  The two quad paths 0 and 5
 * therefore order the square and the diamond clockwise.
 *
 * The harness runs nine paths:
 *
 *     0  the square with the point at the centre: the function returns 1,
 *        the scratch points hold the vertices, and the box is 0 through 10
 *     1  the point to the right of the box: the function returns 0
 *     2  the point to the left of the box: the function returns 0
 *     3  the point above the box: the function returns 0
 *     4  the point below the box: the function returns 0
 *     5  the diamond with the point at (1, 1): the point is inside the box
 *        but outside the first edge, so the function returns 0
 *     6  the translated square with the point at (105, -45): the function
 *        returns 1 and the scratch point 0 holds (100, -50, 7)
 *     7  the negative square with the point at (-5, -5): the function
 *        returns 1 with signed short vertices
 *     8  a square with the z value 42: the function returns 1 and the
 *        scratch z slots hold 42
 *     D  the exit marker
 *
 * The harness uses only BSS state, because the minimal image does not load
 * the initialized data page.
 */
#pragma off (check_stack)

struct point {
    int x;
    int y;
    int z;
};

struct polygon {
    char pad[6];
    short vertex_count;
    unsigned short index[4];
};

struct transform {
    char pad[8];
    int x;
    int y;
    int z;
};

int quad_point_test(struct polygon *poly, struct transform *transform);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct point DAT_000c6d4[4];
int *DAT_000c71c;
int DAT_000c704;
int DAT_000c708;
int DAT_000c70c;
int DAT_000c710;
int DAT_000c728;
int DAT_000c730;

static short vertices[4][3];
static struct polygon poly;
static struct transform transform;
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

static void set_vertex(int index, int x, int y, int z)
{
    vertices[index][0] = (short)x;
    vertices[index][1] = (short)y;
    vertices[index][2] = (short)z;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 4; index++) {
        vertices[index][0] = 0;
        vertices[index][1] = 0;
        vertices[index][2] = 0;
        poly.index[index] = (unsigned short)(index * 6);
        DAT_000c6d4[index].x = 0;
        DAT_000c6d4[index].y = 0;
        DAT_000c6d4[index].z = 0;
    }
    poly.vertex_count = 4;
    DAT_000c71c = (int *)vertices;
    transform.x = 0;
    transform.y = 0;
    transform.z = 0;
    DAT_000c704 = 0;
    DAT_000c708 = 0;
    DAT_000c70c = 0;
    DAT_000c710 = 0;
    DAT_000c728 = 0;
    DAT_000c730 = 0;
}

static void square(void)
{
    set_vertex(0, 0, 0, 0);
    set_vertex(1, 0, 10, 0);
    set_vertex(2, 10, 10, 0);
    set_vertex(3, 10, 0, 0);
}

static void diamond(void)
{
    set_vertex(0, 5, 0, 0);
    set_vertex(1, 0, 5, 0);
    set_vertex(2, 5, 10, 0);
    set_vertex(3, 10, 5, 0);
}

int main(void)
{
    int result;

    reset();
    square();
    DAT_000c728 = 5;
    DAT_000c730 = 5;
    result = quad_point_test(&poly, &transform);
    put(result == 1 && DAT_000c6d4[0].x == 0
        && DAT_000c6d4[2].y == 10 && DAT_000c704 == 10
        && DAT_000c70c == 0 && DAT_000c708 == 10 && DAT_000c710 == 0
        ? '0' : 'x');

    reset();
    square();
    DAT_000c728 = 15;
    DAT_000c730 = 5;
    put(quad_point_test(&poly, &transform) == 0 ? '1' : 'x');

    reset();
    square();
    DAT_000c728 = -5;
    DAT_000c730 = 5;
    put(quad_point_test(&poly, &transform) == 0 ? '2' : 'x');

    reset();
    square();
    DAT_000c728 = 5;
    DAT_000c730 = 15;
    put(quad_point_test(&poly, &transform) == 0 ? '3' : 'x');

    reset();
    square();
    DAT_000c728 = 5;
    DAT_000c730 = -5;
    put(quad_point_test(&poly, &transform) == 0 ? '4' : 'x');

    reset();
    diamond();
    DAT_000c728 = 1;
    DAT_000c730 = 1;
    result = quad_point_test(&poly, &transform);
    put(result == 0 && DAT_000c704 == 10 && DAT_000c70c == 0
        ? '5' : 'x');

    reset();
    square();
    transform.x = 100;
    transform.y = -50;
    transform.z = 7;
    DAT_000c728 = 105;
    DAT_000c730 = -45;
    result = quad_point_test(&poly, &transform);
    put(result == 1 && DAT_000c6d4[0].x == 100
        && DAT_000c6d4[0].y == -50 && DAT_000c6d4[0].z == 7
        && DAT_000c704 == 110 && DAT_000c70c == 100
        ? '6' : 'x');

    reset();
    set_vertex(0, -10, -10, 0);
    set_vertex(1, -10, 0, 0);
    set_vertex(2, 0, 0, 0);
    set_vertex(3, 0, -10, 0);
    DAT_000c728 = -5;
    DAT_000c730 = -5;
    put(quad_point_test(&poly, &transform) == 1 ? '7' : 'x');

    reset();
    set_vertex(0, 0, 0, 42);
    set_vertex(1, 0, 10, 42);
    set_vertex(2, 10, 10, 42);
    set_vertex(3, 10, 0, 42);
    DAT_000c728 = 5;
    DAT_000c730 = 5;
    result = quad_point_test(&poly, &transform);
    put(result == 1 && DAT_000c6d4[3].z == 42 ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
