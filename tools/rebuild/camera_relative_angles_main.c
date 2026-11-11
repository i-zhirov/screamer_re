/* The test harness for the rebuilt `camera_relative_angles` module.
 *
 * The harness holds the screen record at `PTR_DAT_0008a844`, two screen
 * records, two camera records, one object, and one output triple.  The screen
 * record holds the camera pointer at `+0x00`.  The camera record holds the
 * three angle indices at `+0x00`, `+0x04`, and `+0x08`.  The object holds the
 * two angles at `+0x14` and `+0x18`.
 *
 * The paths:
 *
 *     0  the plain case: yaw 50, pitch 974, roll 1747
 *     1  the upper wrap: yaw 2046, pitch 1025, roll 2047
 *     2  a negative object angle and negative camera angles: yaw 2047,
 *        pitch 1024, the unmasked roll 2052
 *     3  the second screen record with the quarter-turn bias: yaw 0, pitch 0,
 *        roll 1023
 *     4  the upper camera limits: yaw 1, pitch 1023, roll 0
 *     5  the output sentinels are overwritten and the object stays unchanged
 *     D  the exit marker
 */
#pragma off (check_stack)

struct camera {
    int angle0;
    int angle1;
    int angle2;
};

struct screen_record {
    struct camera *camera;
};

struct object_record {
    char pad_00[0x14];
    int angle_14;
    int angle_18;
};

struct relative_angles {
    int yaw;
    int pitch;
    int roll;
};

void camera_relative_angles(struct object_record *object,
    struct relative_angles *relative);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct screen_record *PTR_DAT_0008a844;
static struct screen_record screens[2];
static struct camera cameras[2];
static struct object_record object;
static struct relative_angles relative;
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

static void set_camera(int index, int angle0, int angle1, int angle2)
{
    cameras[index].angle0 = angle0;
    cameras[index].angle1 = angle1;
    cameras[index].angle2 = angle2;
}

static void set_object(int angle_14, int angle_18)
{
    object.angle_14 = angle_14;
    object.angle_18 = angle_18;
}

static int matches(int yaw, int pitch, int roll)
{
    return relative.yaw == yaw && relative.pitch == pitch
        && relative.roll == roll;
}

static void clear_sentinels(void)
{
    relative.yaw = 0x11111111;
    relative.pitch = 0x22222222;
    relative.roll = 0x33333333;
}

int main(void)
{
    screens[0].camera = &cameras[0];
    screens[1].camera = &cameras[1];
    PTR_DAT_0008a844 = &screens[0];

    set_camera(0, 100, 200, 300);
    set_object(150, 250);
    clear_sentinels();
    camera_relative_angles(&object, &relative);
    put(matches(50, 974, 1747) ? '0' : 'x');

    set_camera(0, 1, 0, 0);
    set_object(0x7ff, 0x7ff);
    clear_sentinels();
    camera_relative_angles(&object, &relative);
    put(matches(2046, 1025, 2047) ? '1' : 'x');

    set_camera(0, 0, -0x800, -5);
    set_object(-1, 0);
    clear_sentinels();
    camera_relative_angles(&object, &relative);
    put(matches(2047, 1024, 2052) ? '2' : 'x');

    PTR_DAT_0008a844 = &screens[1];
    set_camera(1, 0x400, 0x400, 0x400);
    set_object(0x400, 0);
    clear_sentinels();
    camera_relative_angles(&object, &relative);
    put(matches(0, 0, 1023) ? '3' : 'x');

    set_camera(1, 0x7ff, 0x7ff, 0x7ff);
    set_object(0, 0);
    clear_sentinels();
    camera_relative_angles(&object, &relative);
    put(matches(1, 1023, 0) ? '4' : 'x');

    set_camera(1, 0x10, 0x20, 0x30);
    set_object(0x40, 0x50);
    clear_sentinels();
    camera_relative_angles(&object, &relative);
    put(matches(0x30, (0x20 - 0x50 - 0x400) & 0x7ff, 0x7ff - 0x30)
        && object.angle_14 == 0x40 && object.angle_18 == 0x50 ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
