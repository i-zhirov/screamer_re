/* The test harness for the rebuilt `camera_pose_build` module.
 *
 * The harness holds one view object, the camera pose, and the two float
 * constants.  The module writes the three angle indices and the three position
 * values of the camera pose.
 *
 * The harness runs these paths:
 *
 *     0  the transition flag set: the angle takes the base plus the delta, the
 *        angle pi/2 reaches 512, pi reaches 1024, and two pi wraps to 0, and
 *        the position takes the base plus the delta
 *     1  the transition flag clear: the angle takes the base alone, and the
 *        delta stays unused, so pi reaches 1024, minus pi wraps to 1024, and
 *        pi/2 reaches 512
 *     2  the transition flag set with 1.0 and -1.0: the truncation toward zero
 *        reaches 325 and 1723, not 324 or 1722
 *     3  the transition flag clear with the position zero: the pose keeps the
 *        base position
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because an FPU memory
 * operand of a literal reads the constant pool at a low address in this
 * toolchain.
 */
#pragma off (check_stack)

#define HALF_PI_BITS 0x3fc90fdb
#define PI_BITS 0x40490fdb
#define TWO_PI_BITS 0x40c90fdb
#define NEG_PI_BITS 0xc0490fdb
#define ONE_BITS 0x3f800000
#define NEG_ONE_BITS 0xbf800000
#define SCALE_BITS 0x44800000

struct camera_pose {
    int angle[3];
    int position[3];
};

struct view_object {
    int position[3];
    int reserved_0c[3];
    int copy[3];
    int delta[3];
    float angle[3];
    float angle_delta[3];
    float saved_angle[3];
    int transition;
    int use_delta;
};

void camera_pose_build(struct view_object *view);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct camera_pose camera_pose;
float view_angle_scale;
float view_angle_pi;

static struct view_object view;
static char text[8];

static void set_bits_float(float *value, unsigned int pattern)
{
    *(unsigned int *)value = pattern;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 3; index++) {
        view.position[index] = 0;
        view.reserved_0c[index] = 0;
        view.copy[index] = 0;
        view.delta[index] = 0;
        view.angle[index] = 0.0f;
        view.angle_delta[index] = 0.0f;
        view.saved_angle[index] = 0.0f;
        camera_pose.angle[index] = 0;
        camera_pose.position[index] = 0;
    }
    view.transition = 0;
    view.use_delta = 0;
    set_bits_float(&view_angle_scale, SCALE_BITS);
    set_bits_float(&view_angle_pi, PI_BITS);
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

static int angle_are(int a0, int a1, int a2)
{
    return camera_pose.angle[0] == a0
        && camera_pose.angle[1] == a1
        && camera_pose.angle[2] == a2;
}

static int position_are(int p0, int p1, int p2)
{
    return camera_pose.position[0] == p0
        && camera_pose.position[1] == p1
        && camera_pose.position[2] == p2;
}

int main(void)
{
    int ok;

    reset();
    view.use_delta = 1;
    view.position[0] = 100;
    view.position[1] = 200;
    view.position[2] = 300;
    view.delta[0] = 10;
    view.delta[1] = 20;
    view.delta[2] = 30;
    set_bits_float(&view.angle[0], 0);
    set_bits_float(&view.angle[1], 0);
    set_bits_float(&view.angle[2], PI_BITS);
    set_bits_float(&view.angle_delta[0], HALF_PI_BITS);
    set_bits_float(&view.angle_delta[1], PI_BITS);
    set_bits_float(&view.angle_delta[2], PI_BITS);
    camera_pose_build(&view);
    ok = angle_are(512, 1024, 0)
        && position_are(110, 220, 330);
    put(ok ? '0' : 'x');

    reset();
    view.use_delta = 0;
    view.position[0] = -5;
    view.position[1] = -6;
    view.position[2] = -7;
    view.delta[0] = 9;
    view.delta[1] = 9;
    view.delta[2] = 9;
    set_bits_float(&view.angle[0], PI_BITS);
    set_bits_float(&view.angle[1], NEG_PI_BITS);
    set_bits_float(&view.angle[2], HALF_PI_BITS);
    set_bits_float(&view.angle_delta[0], 0x41200000);
    set_bits_float(&view.angle_delta[1], 0x41200000);
    set_bits_float(&view.angle_delta[2], 0x41200000);
    camera_pose_build(&view);
    ok = angle_are(1024, 1024, 512)
        && position_are(-5, -6, -7);
    put(ok ? '1' : 'x');

    reset();
    view.use_delta = 1;
    view.position[0] = 7;
    view.position[1] = 8;
    view.position[2] = 9;
    set_bits_float(&view.angle[0], ONE_BITS);
    set_bits_float(&view.angle[1], NEG_ONE_BITS);
    set_bits_float(&view.angle[2], 0);
    camera_pose_build(&view);
    ok = angle_are(325, 1723, 0)
        && position_are(7, 8, 9);
    put(ok ? '2' : 'x');

    reset();
    view.use_delta = 0;
    view.position[0] = 0;
    view.position[1] = 0;
    view.position[2] = 0;
    view.delta[0] = 40;
    view.delta[1] = 50;
    view.delta[2] = 60;
    set_bits_float(&view.angle[0], TWO_PI_BITS);
    camera_pose_build(&view);
    ok = angle_are(0, 0, 0)
        && position_are(0, 0, 0);
    put(ok ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
