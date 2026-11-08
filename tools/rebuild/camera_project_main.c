/* The test harness for the rebuilt `camera_project` module.
 *
 * The harness holds the camera pointer, two camera records, the angle table,
 * and the twelve output globals.  The angle table holds 0x800 entries of two
 * signed 16-bit values, the cosine at `+0x00` and the sine at `+0x04`.
 *
 * The harness runs these paths:
 *
 *     0  three angles with positive, negative, and odd values: every output
 *        takes the arithmetic shift of its table value, including the two
 *        negative odd shifts, and the complement index is 0x7ff - angle
 *     1  the table limits 0x7fff and -0x8000: the shift and the complement
 *        reuse the shared entries
 *     2  a second call with one new angle: only that angle and its complement
 *        change, and the other eight outputs keep their values
 *     3  a second camera pointer and a zero table: every output clears
 *     D  the exit marker
 */
#pragma off (check_stack)

void camera_project(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define MIRROR_BASE 0x7ff
#define TABLE_SIZE 0x800

struct camera {
    int angle0;
    int angle1;
    int angle2;
};

struct projection_angle {
    short cosine;
    short reserved_00;
    short sine;
    short reserved_04;
};

struct camera *camera_state;
struct projection_angle projection_angles[TABLE_SIZE];
int projection_cos_0;
int projection_sin_0;
int projection_cos_1;
int projection_sin_1;
int projection_cos_2;
int projection_sin_2;
int projection_complement_cos_0;
int projection_complement_sin_0;
int projection_complement_cos_1;
int projection_complement_sin_1;
int projection_complement_cos_2;
int projection_complement_sin_2;

static struct camera cameras[2];
static char text[8];

static void set_angle(int index, int cosine, int sine)
{
    projection_angles[index].cosine = (short)cosine;
    projection_angles[index].sine = (short)sine;
}

static void clear_table(void)
{
    int index;

    for (index = 0; index < TABLE_SIZE; index++) {
        projection_angles[index].cosine = 0;
        projection_angles[index].sine = 0;
    }
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

int main(void)
{
    int ok;

    clear_table();
    cameras[0].angle0 = 3;
    cameras[0].angle1 = 0x20;
    cameras[0].angle2 = 0x101;
    camera_state = &cameras[0];
    set_angle(3, 1000, -2000);
    set_angle(0x20, -4, 7);
    set_angle(0x101, 0x1234, -0x1235);
    set_angle(MIRROR_BASE - 3, -5, 9);
    set_angle(MIRROR_BASE - 0x20, -0x4000, 0x3fff);
    set_angle(MIRROR_BASE - 0x101, 1, -1);
    camera_project();
    put(projection_cos_0 == 500
        && projection_sin_0 == -1000
        && projection_cos_1 == -2
        && projection_sin_1 == 3
        && projection_cos_2 == 2330
        && projection_sin_2 == -2331
        && projection_complement_cos_0 == -3
        && projection_complement_sin_0 == 4
        && projection_complement_cos_1 == -8192
        && projection_complement_sin_1 == 8191
        && projection_complement_cos_2 == 0
        && projection_complement_sin_2 == -1 ? '0' : 'x');

    clear_table();
    cameras[0].angle0 = 0;
    cameras[0].angle1 = 0x7ff;
    cameras[0].angle2 = 0x400;
    set_angle(0, 0x7fff, -0x8000);
    set_angle(0x7ff, -0x8000, 0x7fff);
    set_angle(0x400, 12345, -12345);
    set_angle(0x3ff, -7, 11);
    camera_project();
    put(projection_cos_0 == 16383
        && projection_sin_0 == -16384
        && projection_cos_1 == -16384
        && projection_sin_1 == 16383
        && projection_cos_2 == 6172
        && projection_sin_2 == -6173
        && projection_complement_cos_0 == -16384
        && projection_complement_sin_0 == 16383
        && projection_complement_cos_1 == 16383
        && projection_complement_sin_1 == -16384
        && projection_complement_cos_2 == -4
        && projection_complement_sin_2 == 5 ? '1' : 'x');

    clear_table();
    cameras[0].angle0 = 0x17;
    cameras[0].angle1 = 0x18;
    cameras[0].angle2 = 0x19;
    camera_state = &cameras[0];
    set_angle(0x17, 100, 200);
    set_angle(0x18, 300, 400);
    set_angle(0x19, 500, 600);
    set_angle(MIRROR_BASE - 0x17, 700, 800);
    set_angle(MIRROR_BASE - 0x18, 900, 1000);
    set_angle(MIRROR_BASE - 0x19, 1100, 1200);
    camera_project();
    ok = projection_cos_0 == 50
        && projection_sin_0 == 100
        && projection_cos_1 == 150
        && projection_sin_1 == 200
        && projection_cos_2 == 250
        && projection_sin_2 == 300
        && projection_complement_cos_0 == 350
        && projection_complement_sin_0 == 400
        && projection_complement_cos_1 == 450
        && projection_complement_sin_1 == 500
        && projection_complement_cos_2 == 550
        && projection_complement_sin_2 == 600;

    cameras[0].angle0 = 0x1a;
    set_angle(0x1a, -600, 800);
    set_angle(MIRROR_BASE - 0x1a, -900, 1000);
    camera_project();
    put(ok
        && projection_cos_0 == -300
        && projection_sin_0 == 400
        && projection_complement_cos_0 == -450
        && projection_complement_sin_0 == 500
        && projection_cos_1 == 150
        && projection_sin_1 == 200
        && projection_cos_2 == 250
        && projection_sin_2 == 300
        && projection_complement_cos_1 == 450
        && projection_complement_sin_1 == 500
        && projection_complement_cos_2 == 550
        && projection_complement_sin_2 == 600 ? '2' : 'x');

    clear_table();
    cameras[1].angle0 = 0x100;
    cameras[1].angle1 = 0x200;
    cameras[1].angle2 = 0x300;
    camera_state = &cameras[1];
    camera_project();
    put(projection_cos_0 == 0
        && projection_sin_0 == 0
        && projection_cos_1 == 0
        && projection_sin_1 == 0
        && projection_cos_2 == 0
        && projection_sin_2 == 0
        && projection_complement_cos_0 == 0
        && projection_complement_sin_0 == 0
        && projection_complement_cos_1 == 0
        && projection_complement_sin_1 == 0
        && projection_complement_cos_2 == 0
        && projection_complement_sin_2 == 0 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
