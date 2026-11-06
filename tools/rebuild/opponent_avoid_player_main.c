/* The test harness for the rebuilt `opponent_avoid_player` module.
 *
 * The harness holds the opponent object, the player position, the player body
 * points, the player radius, the player speed, and the opponent speed.  It runs
 * the module on each path and checks the result fields.
 *
 * The heading is zero on every path, so the look-ahead point is the object
 * position minus three radii on the z axis and the sine and cosine values are
 * exact.  The paths cover the two far guards, the two far body points, the
 * speed gate, the first point, the second point, both points, the radius
 * scaling, the 4999-unit guard boundary, and the distance equal to the
 * avoidance radius:
 *
 *     0  the look-ahead point is 5000 units or more from the player in x
 *     1  the look-ahead point is 5000 units or more from the player in z
 *     2  both body points stay outside the avoidance radius
 *     3  the first point is inside but the opponent speed stays at the player
 *        speed
 *     4  the first point pushes the local position along the positive offsets
 *     5  the second point pushes the local position along the negative offsets
 *        and the negative sums truncate toward zero
 *     6  both points push in turn; the near-zero sums truncate to zero
 *     7  the radius 200 sets the look-ahead point and the avoidance radius 450
 *     8  the x guard accepts the distance 4999
 *     9  the point at the avoidance radius does not push
 *     D  the exit marker
 *
 * The float values arrive through exact bit patterns, because a float literal
 * in this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void opponent_avoid_player(char *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int player_world_x;
int player_world_z;
float player_speed;
int player_radius;
int player_point0_x;
int player_point0_z;
int player_point1_x;
int player_point1_z;

static char text[8];
static char object[0x500];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void clear_bytes(char *p, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        p[index] = 0;
    }
}

static void set_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *p = value.f;
}

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int get_int(int offset)
{
    return *(int *)(object + offset);
}

static void reset_case(void)
{
    clear_bytes(object, 0x500);

    player_world_x = 0;
    player_world_z = 0;
    player_speed = 0.0f;
    player_radius = 0;
    player_point0_x = 0;
    player_point0_z = 0;
    player_point1_x = 0;
    player_point1_z = 0;
}

static void set_motion(int radius, int x, int z, unsigned int speed)
{
    set_int(0x44c, radius);
    set_int(0x498, x);
    set_int(0x4a0, z);
    set_float((float *)(object + 0x4a8), 0x00000000);
    set_float((float *)(object + 0x418), speed);
}

static void set_player(int x, int z, int radius, unsigned int speed,
                       int point0_x, int point0_z,
                       int point1_x, int point1_z)
{
    player_world_x = x;
    player_world_z = z;
    player_radius = radius;
    set_float(&player_speed, speed);
    player_point0_x = point0_x;
    player_point0_z = point0_z;
    player_point1_x = point1_x;
    player_point1_z = point1_z;
}

int main(void)
{
    reset_case();
    set_motion(100, 500, 1000, 0x41a00000);
    set_int(0x454, 7);
    set_int(0x45c, 9);
    set_int(0x484, 0x5678);
    set_int(0x488, 0x1234);
    set_int(0x48c, 0x9abc);
    set_int(0x494, 0xcdef);
    set_player(6500, 700, 50, 0x41200000, 0, 0, 0, 0);
    opponent_avoid_player(object);
    put(get_int(0x488) == 0x1234 && get_int(0x484) == 0x5678
        && get_int(0x48c) == 0x9abc && get_int(0x494) == 0xcdef
        && get_int(0x454) == 7 && get_int(0x45c) == 9
        ? '0' : 'x');

    reset_case();
    set_motion(100, 500, 1000, 0x41a00000);
    set_int(0x454, 7);
    set_int(0x45c, 9);
    set_int(0x484, 0x5678);
    set_int(0x488, 0x1234);
    set_int(0x48c, 0x9abc);
    set_int(0x494, 0xcdef);
    set_player(500, 6700, 50, 0x41200000, 0, 0, 0, 0);
    opponent_avoid_player(object);
    put(get_int(0x488) == 0x1234 && get_int(0x484) == 0x5678
        && get_int(0x48c) == 0x9abc && get_int(0x494) == 0xcdef
        && get_int(0x454) == 7 && get_int(0x45c) == 9
        ? '1' : 'x');

    reset_case();
    set_motion(100, 500, 1000, 0x41a00000);
    set_player(500, 700, 50, 0x41200000, 5000, 5000, 500, 0);
    opponent_avoid_player(object);
    put(get_int(0x488) == 250 && get_int(0x484) == 700
        && get_int(0x48c) == 0 && get_int(0x494) == 700
        && get_int(0x454) == 0 && get_int(0x45c) == 0
        ? '2' : 'x');

    reset_case();
    set_motion(100, 500, 1000, 0x40a00000);
    set_player(500, 700, 50, 0x41200000, 470, 660, 500, 0);
    opponent_avoid_player(object);
    put(get_int(0x488) == 250 && get_int(0x484) == 700
        && get_int(0x454) == 0 && get_int(0x45c) == 0
        ? '3' : 'x');

    reset_case();
    set_motion(100, 500, 1000, 0x41a00000);
    set_player(500, 700, 50, 0x41200000, 470, 660, 500, 0);
    opponent_avoid_player(object);
    put(get_int(0x488) == 250 && get_int(0x484) == 700
        && get_int(0x454) == 117 && get_int(0x45c) == 156
        ? '4' : 'x');

    reset_case();
    set_motion(100, 500, 1000, 0x41a00000);
    set_player(500, 700, 50, 0x41200000, 500, 0, 530, 740);
    opponent_avoid_player(object);
    put(get_int(0x488) == 250 && get_int(0x484) == 51
        && get_int(0x48c) == -30 && get_int(0x494) == -40
        && get_int(0x454) == -117 && get_int(0x45c) == -156
        ? '5' : 'x');

    reset_case();
    set_motion(100, 500, 1000, 0x41a00000);
    set_player(500, 700, 50, 0x41200000, 470, 660, 530, 740);
    opponent_avoid_player(object);
    put(get_int(0x488) == 250 && get_int(0x484) == 51
        && get_int(0x454) == 0 && get_int(0x45c) == 0
        ? '6' : 'x');

    reset_case();
    set_motion(200, 500, 1000, 0x41a00000);
    set_player(500, 400, 50, 0x41200000, 500, 450, 500, -1000);
    opponent_avoid_player(object);
    put(get_int(0x488) == 450 && get_int(0x484) == 1400
        && get_int(0x454) == 0 && get_int(0x45c) == -391
        ? '7' : 'x');

    reset_case();
    set_motion(100, 500, 1000, 0x41a00000);
    set_player(5499, 700, 50, 0x41200000, 5000, 5000, 500, 0);
    opponent_avoid_player(object);
    put(get_int(0x488) == 250 && get_int(0x484) == 700
        && get_int(0x454) == 0 && get_int(0x45c) == 0
        ? '8' : 'x');

    reset_case();
    set_motion(100, 500, 1000, 0x41a00000);
    set_player(500, 700, 50, 0x41200000, 500, 450, 500, 0);
    opponent_avoid_player(object);
    put(get_int(0x488) == 250 && get_int(0x484) == 700
        && get_int(0x454) == 0 && get_int(0x45c) == 0
        ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
