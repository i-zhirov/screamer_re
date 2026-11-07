/* The test harness for the rebuilt `sound_event_render` module.
 *
 * The harness holds the view record, the pointer record Q, the output buffer,
 * the two helpers, and the recorded tail path.  The prepare helper counts its
 * calls.  The frame selector records its three arguments.  The module records
 * the selected tail in `render_path` instead of a call into the code outside
 * the 100-instruction window.
 *
 * The harness runs eight paths:
 *
 *     0  the near path adds the signed short offsets 1, 2, and 3 to the body
 *        position and scales 101, 202, and 303 to 121, 242, and 363; the
 *        distance is 13
 *     1  the far path takes the depth 0 and the bias -20, so the sum stays
 *        below -8 and the clipped value stays above the limit 0
 *     2  the clipped path takes the same depth and bias with the limit 20
 *     3  the sum -8 takes the near path, the equality edge of the compare
 *     4  the coordinate 30000 overflows the signed 32-bit product, so all
 *        three coordinates keep their original values
 *     5  the flag bit 0x2000 calls the prepare helper, and the other bits do
 *        not
 *     6  the negative coordinates round toward negative infinity, so -1, -2,
 *        and -3 scale to -2, -3, and -4
 *     7  the distance 26 comes from the axis values 6, 8, and 24
 *     D  the exit marker
 */
#pragma off (check_stack)

void sound_event_render(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define VIEW_SIZE 0x200
#define ROOT_SIZE 16
#define OUTPUT_SIZE 0x100

#define TAIL_NEAR 1
#define TAIL_CLIPPED 2
#define TAIL_FAR 3

static char view[VIEW_SIZE];
static char root[ROOT_SIZE];
static char text[8];

void *render_view;
int render_flags;
int render_depth_limit;
int render_path;

static int prepare_calls;
static int select_calls;
static int select_x;
static int select_y;
static int select_z;

void render_prepare(void)
{
    prepare_calls++;
}

void render_frame_select(int x, int y, int z)
{
    select_calls++;
    select_x = x;
    select_y = y;
    select_z = z;
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

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static void set_int(void *base, int offset, int value)
{
    *(int *)((char *)base + offset) = value;
}

static short get_short(void *base, int offset)
{
    return *(short *)((char *)base + offset);
}

static int get_int(void *base, int offset)
{
    return *(int *)((char *)base + offset);
}

static void set_short(void *base, int offset, int value)
{
    *(short *)((char *)base + offset) = (short)value;
}

static void setup(void)
{
    int index;

    for (index = 0; index < VIEW_SIZE; index++) {
        view[index] = 0;
    }
    for (index = 0; index < ROOT_SIZE; index++) {
        root[index] = 0;
    }
    *(char **)view = root;
    render_view = view;
    render_flags = 0;
    render_depth_limit = 0;
    render_path = 0;
    prepare_calls = 0;
    select_calls = 0;
    select_x = 0;
    select_y = 0;
    select_z = 0;
}

static void set_view(int x, int y, int depth)
{
    set_int(view, 0x13c, x);
    set_int(view, 0x140, y);
    set_int(view, 0x144, depth);
}

static void set_body(int x, int y, int z)
{
    set_int(view, 0x16c, x);
    set_int(view, 0x170, y);
    set_int(view, 0x174, z);
}

static void set_root(int bias, int first, int second, int third)
{
    set_int(root, 0, bias);
    set_short(root, 4, first);
    set_short(root, 6, second);
    set_short(root, 8, third);
}

int main(void)
{
    setup();
    set_view(3, 4, 0x74);
    set_body(100, 200, 300);
    set_root(10, 1, 2, 3);
    sound_event_render();
    check('0', get_int(view, 0x1f0) == 13
        && get_short(root, 4) == 1
        && select_calls == 1
        && select_x == 121
        && select_y == 242
        && select_z == 363
        && prepare_calls == 0
        && render_path == TAIL_NEAR);

    setup();
    set_view(0, 0, 0);
    set_body(10, 20, 30);
    set_root(-20, 0, 0, 0);
    sound_event_render();
    check('1', get_int(view, 0x1f0) == 128
        && select_calls == 1
        && select_x == 11
        && select_y == 23
        && select_z == 35
        && render_path == TAIL_FAR);

    setup();
    set_view(0, 0, 0);
    set_body(10, 20, 30);
    set_root(-20, 0, 0, 0);
    render_depth_limit = 20;
    sound_event_render();
    check('2', select_x == 11
        && select_y == 23
        && select_z == 35
        && render_path == TAIL_CLIPPED);

    setup();
    set_view(2, 3, 0x7a);
    set_body(5, 6, 7);
    set_root(-130, 0, 0, 0);
    sound_event_render();
    check('3', get_int(view, 0x1f0) == 7
        && select_x == 5
        && select_y == 7
        && select_z == 8
        && render_path == TAIL_NEAR);

    setup();
    set_view(0, 0, 0);
    set_body(30000, 20000, 10000);
    set_root(10, 0, 0, 0);
    sound_event_render();
    check('4', select_calls == 1
        && select_x == 30000
        && select_y == 20000
        && select_z == 10000
        && render_path == TAIL_NEAR);

    setup();
    set_view(1, 2, 0x7e);
    set_body(3, 4, 5);
    set_root(10, 0, 0, 0);
    render_flags = 0x2000;
    sound_event_render();
    check('5', get_int(view, 0x1f0) == 3
        && select_x == 3
        && select_y == 4
        && select_z == 5
        && prepare_calls == 1
        && render_path == TAIL_NEAR);

    setup();
    set_view(0, 0, 0);
    set_body(-1, -2, -3);
    set_root(10, 0, 0, 0);
    sound_event_render();
    check('6', select_x == -2
        && select_y == -3
        && select_z == -4
        && render_path == TAIL_NEAR);

    setup();
    set_view(6, 8, 0x68);
    set_body(2, 3, 4);
    set_root(10, 0, 0, 0);
    sound_event_render();
    check('7', get_int(view, 0x1f0) == 26
        && select_x == 2
        && select_y == 3
        && select_z == 4
        && render_path == TAIL_NEAR);

    put('D');
    quit();
    return 0;
}
