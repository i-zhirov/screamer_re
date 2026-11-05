/* The test harness for the rebuilt `player_view_setup` module.
 *
 * The harness holds the seven view slots, the kind flags, the input bytes, the
 * car records, and the player view.  The stubs record their arguments and count
 * their calls.
 *
 * The program runs these paths:
 *
 *     0  the chosen slot 2: the screen mode, the render call, the field copies,
 *        and the chosen bit clear
 *     1  the kind flag selects slot 0 and records the free slot
 *     2  the chosen slot 1 falls back to slot 0 behind the input flag 0x20
 *     3  the value flag 0x74 runs the car camera and skips the renderer
 *     4  the chosen slot 4 runs the car camera and the camera scan
 *     5  the camera scan wraps the counter
 *     6  the value flag 0x7c takes the distance branch with truncation
 *     7  no slot is eligible, so the function returns early
 *     8  the video mode 1 becomes 2
 *     9  the input bit 0x20 blocks the kind flag, so the chosen slot 5 runs
 *     D  the done marker
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness builds every text in writable data at run time, so the test needs no
 * initialized data page.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

struct view_slot {
    int kind;
    int mode;
    int distance;
    int video;
    int chosen;
};

struct player_view {
    int x;
    int y;
    int z;
    int field_0c;
    int field_10;
    int field_14;
    int copy_x;
    int copy_y;
    int copy_z;
    int field_24;
    int field_28;
    int field_2c;
    float angle_x;
    float angle_y;
    float angle_z;
    int field_3c;
    int field_40;
    int field_44;
    float saved_x;
    float saved_y;
    float saved_z;
    int flag_54;
    int flag_58;
    int field_5c;
    int field_60;
    int field_64;
    int field_68;
    int flag_6c;
    int field_70;
    int flag_74;
    int field_78;
    int flag_7c;
    int flag_80;
    int index;
    int mode;
    int video;
    int distance;
    int car;
};

void player_view_setup(struct player_view *view);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct view_slot DAT_00101da4[7];
unsigned char DAT_000f003d[16];
unsigned char DAT_00118e1c;
unsigned char DAT_00118e1d;
int DAT_00118e6c;
unsigned char DAT_0011d274;
int DAT_001100c0;
int DAT_0011d2a8;
int DAT_0010f8ec;
int DAT_00110a18;
int DAT_00110a1c;
int DAT_0011100c[0x1000];
unsigned char DAT_00110ab8[0x3000];
double view_distance_scale;

static struct player_view view;
static char text[8];

static int screen_calls;
static unsigned int screen_width;
static unsigned int screen_height;
static unsigned int screen_value;
static int render_calls;
static struct player_view *render_arg;
static int car_camera_calls;
static void *car_camera_arg;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void screen_mode_set(unsigned int width, unsigned int height,
                     unsigned int value)
{
    screen_width = width;
    screen_height = height;
    screen_value = value;
    screen_calls = screen_calls + 1;
}

void view_render(struct player_view *view_arg)
{
    render_arg = view_arg;
    render_calls = render_calls + 1;
}

void car_camera(void *car)
{
    car_camera_arg = car;
    car_camera_calls = car_camera_calls + 1;
}

static void clear_view(void)
{
    unsigned char *bytes;
    int index;

    bytes = (unsigned char *)&view;
    for (index = 0; index < sizeof(view); ++index) {
        bytes[index] = 0;
    }
}

static void reset(void)
{
    int index;

    clear_view();
    for (index = 0; index < 7; ++index) {
        DAT_00101da4[index].kind = 0;
        DAT_00101da4[index].mode = 0;
        DAT_00101da4[index].distance = 0x190;
        DAT_00101da4[index].video = 2;
        DAT_00101da4[index].chosen = 0;
    }
    DAT_00101da4[1].kind = 1;
    DAT_00101da4[1].distance = 0x108;
    DAT_00101da4[1].video = 1;
    DAT_00101da4[2].kind = 2;
    DAT_00101da4[2].mode = 1;
    DAT_00101da4[3].kind = 9;
    DAT_00101da4[3].mode = 3;
    DAT_00101da4[3].video = 0;
    DAT_00101da4[4].kind = 9;
    DAT_00101da4[4].mode = 8;
    DAT_00101da4[4].video = 0;
    DAT_00101da4[5].kind = 9;
    DAT_00101da4[5].mode = 6;
    DAT_00101da4[6].kind = 3;
    DAT_00101da4[6].mode = 9;
    for (index = 0; index < 16; ++index) {
        DAT_000f003d[index] = 0;
    }
    DAT_00118e1c = 0x02;
    DAT_00118e1d = 0;
    DAT_00118e6c = 0;
    DAT_0011d274 = 0;
    DAT_001100c0 = 0;
    DAT_0011d2a8 = 0x77;
    DAT_0010f8ec = 1;
    DAT_00110a18 = 0;
    DAT_00110a1c = 0;
    for (index = 0; index < 0x1000; ++index) {
        DAT_0011100c[index] = 0;
    }
    view.index = 0x7f;
    view.x = 0x11;
    view.y = 0x22;
    view.z = 0x33;
    view.angle_x = 1.5f;
    view.angle_y = 2.5f;
    view.angle_z = 3.5f;
    view_distance_scale = 0.65;
    screen_calls = 0;
    screen_width = 0;
    screen_height = 0;
    screen_value = 0;
    render_calls = 0;
    render_arg = 0;
    car_camera_calls = 0;
    car_camera_arg = 0;
}

static int run_chosen(void)
{
    reset();
    DAT_00101da4[2].chosen = 1;
    player_view_setup(&view);
    return screen_calls == 1
        && screen_width == 0x140
        && screen_height == 0x190
        && screen_value == 0x104
        && DAT_001100c0 == 0
        && view.distance == 0x190
        && view.video == 2
        && view.mode == 1
        && view.index == 2
        && view.flag_54 == 1
        && view.flag_58 == 1
        && view.flag_6c == 1
        && view.flag_80 == 1
        && view.copy_x == 0x11
        && view.copy_y == 0x22
        && view.copy_z == 0x33
        && view.saved_x == 1.5f
        && view.saved_y == 2.5f
        && view.saved_z == 3.5f
        && render_calls == 1
        && render_arg == &view
        && car_camera_calls == 0
        && DAT_0011d2a8 == 0x77
        && DAT_00101da4[2].chosen == 0;
}

static int run_kind(void)
{
    reset();
    DAT_000f003d[0] = 1;
    player_view_setup(&view);
    return render_calls == 1
        && view.index == 0
        && view.mode == 0
        && view.distance == 0x190
        && DAT_0011d2a8 == 0;
}

static int run_special(void)
{
    reset();
    DAT_00101da4[1].chosen = 1;
    DAT_0011d274 = 0x20;
    player_view_setup(&view);
    return view.index == 0
        && view.mode == 0
        && view.distance == 0x190
        && screen_value == 0x104
        && DAT_00101da4[1].chosen == 0
        && DAT_0011d2a8 == 0;
}

static int run_car_only(void)
{
    reset();
    DAT_00101da4[3].chosen = 1;
    view.flag_74 = 1;
    view.car = 2;
    player_view_setup(&view);
    return render_calls == 0
        && car_camera_calls == 1
        && car_camera_arg == (void *)(DAT_00110ab8 + 2 * 0x590)
        && view.index == 3
        && view.mode == 3
        && DAT_00101da4[3].chosen == 0;
}

static int run_slot4(void)
{
    reset();
    DAT_00101da4[4].chosen = 1;
    view.car = 1;
    DAT_0010f8ec = 3;
    DAT_00110a18 = 0;
    DAT_00110a1c = 0;
    DAT_0011100c[0] = 5;
    DAT_0011100c[1 * 0x164] = 5;
    DAT_0011100c[2 * 0x164] = 7;
    player_view_setup(&view);
    return car_camera_calls == 1
        && car_camera_arg == (void *)(DAT_00110ab8 + 1 * 0x590)
        && render_calls == 0
        && view.index == 4
        && view.mode == 8
        && view.flag_7c == 1
        && DAT_00110a1c == 2
        && DAT_00101da4[4].chosen == 0;
}

static int run_wrap(void)
{
    reset();
    DAT_00101da4[4].chosen = 1;
    view.car = 0;
    DAT_0010f8ec = 3;
    DAT_00110a18 = 0;
    DAT_00110a1c = 2;
    DAT_0011100c[0] = 1;
    DAT_0011100c[1 * 0x164] = 1;
    DAT_0011100c[2 * 0x164] = 1;
    player_view_setup(&view);
    return car_camera_calls == 1
        && view.flag_7c == 1
        && DAT_00110a1c == 0
        && DAT_00101da4[4].chosen == 0;
}

static int run_distance(void)
{
    reset();
    DAT_00101da4[1].chosen = 1;
    view.flag_7c = 1;
    player_view_setup(&view);
    return screen_calls == 1
        && screen_width == 0x140
        && screen_height == 0x108
        && screen_value == 171
        && DAT_001100c0 == 1
        && render_calls == 0
        && car_camera_calls == 0
        && view.index == 1
        && view.mode == 0
        && view.copy_x == 0
        && DAT_00101da4[1].chosen == 0;
}

static int run_none(void)
{
    reset();
    view.copy_x = 0x1234;
    player_view_setup(&view);
    return screen_calls == 0
        && render_calls == 0
        && car_camera_calls == 0
        && view.copy_x == 0x1234
        && view.index == 0x7f;
}

static int run_video(void)
{
    reset();
    DAT_00101da4[2].chosen = 1;
    DAT_001100c0 = 1;
    player_view_setup(&view);
    return DAT_001100c0 == 2;
}

static int run_blocked(void)
{
    reset();
    DAT_000f003d[0] = 1;
    DAT_00118e1d = 0x20;
    DAT_00101da4[5].chosen = 1;
    player_view_setup(&view);
    return view.index == 5
        && view.mode == 6
        && view.distance == 0x190
        && DAT_0011d2a8 == 0x77;
}

int main(void)
{
    put(run_chosen() ? '0' : 'x');
    put(run_kind() ? '1' : 'x');
    put(run_special() ? '2' : 'x');
    put(run_car_only() ? '3' : 'x');
    put(run_slot4() ? '4' : 'x');
    put(run_wrap() ? '5' : 'x');
    put(run_distance() ? '6' : 'x');
    put(run_none() ? '7' : 'x');
    put(run_video() ? '8' : 'x');
    put(run_blocked() ? '9' : 'x');
    put('D');
    quit();
    return 0;
}
