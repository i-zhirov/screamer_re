/* The test harness for the rebuilt `credits_fade_out` module.
 *
 * The harness holds the frame delta `DAT_00118e04`, the fade level
 * `DAT_0011d224`, the three view offsets `DAT_0011d2c0`, `DAT_0011d2bc`, and
 * `DAT_0011d2c4`, the session state `DAT_0011ce44`, and the three callee stubs.
 * The `view_move` stub records the three arguments, the `mouse_y_set` stub
 * records the value, and the `video_present` stub records the scale and the
 * offset.
 *
 * The harness runs four paths:
 *
 *     0  a positive fade: the level falls from 1.0 to 0.75, the view moves to
 *        the scaled offsets, the mouse value follows, and the state stays 0
 *     1  a fade that reaches zero: the view moves to zero and the state stays 0
 *     2  a fade below zero: the black present and the zero view run, the mouse
 *        helper is skipped, and the state becomes 4
 *     3  negative offsets: the scaled values truncate toward zero
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void credits_fade_out(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float DAT_00118e04;
float DAT_0011d224;
int DAT_0011d2bc;
int DAT_0011d2c0;
int DAT_0011d2c4;
int DAT_0011ce44;

static char text[8];
static int view_calls;
static int view_x[8];
static int view_y[8];
static int view_z[8];
static int target_calls;
static int target_value;
static int present_calls;
static float present_scale[8];
static int present_offset[8];

void view_move(int x, int y, int z)
{
    int index;

    index = view_calls;
    if (index < 8) {
        view_x[index] = x;
        view_y[index] = y;
        view_z[index] = z;
    }
    view_calls = index + 1;
}

void mouse_y_set(int value)
{
    target_calls = target_calls + 1;
    target_value = value;
}

void video_present(float scale, int offset)
{
    int index;

    index = present_calls;
    if (index < 8) {
        present_scale[index] = scale;
        present_offset[index] = offset;
    }
    present_calls = index + 1;
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

static void reset(float fade, float delta, int x, int y, int z)
{
    int index;

    DAT_00118e04 = delta;
    DAT_0011d224 = fade;
    DAT_0011d2c0 = x;
    DAT_0011d2bc = y;
    DAT_0011d2c4 = z;
    DAT_0011ce44 = 0;
    view_calls = 0;
    target_calls = 0;
    target_value = -12345;
    present_calls = 0;
    for (index = 0; index < 8; index++) {
        view_x[index] = 0;
        view_y[index] = 0;
        view_z[index] = 0;
        present_scale[index] = -1.0f;
        present_offset[index] = -1;
    }
}

static int view_is(int index, int x, int y, int z)
{
    return index < view_calls
        && view_x[index] == x
        && view_y[index] == y
        && view_z[index] == z;
}

static int present_is(int index, float scale, int offset)
{
    return index < present_calls
        && present_scale[index] == scale
        && present_offset[index] == offset;
}

int main(void)
{
    reset(1.0f, 0.25f, 100, 200, 300);
    credits_fade_out();
    put(view_calls == 1 && view_is(0, 75, 150, 225)
        && target_calls == 1 && target_value == 150
        && present_calls == 1 && present_is(0, 0.75f, 0)
        && DAT_0011d224 == 0.75f && DAT_0011ce44 == 0 ? '0' : 'x');

    reset(0.5f, 0.5f, 100, 200, 300);
    credits_fade_out();
    put(view_calls == 1 && view_is(0, 0, 0, 0)
        && target_calls == 1 && target_value == 0
        && present_calls == 1 && present_is(0, 0.0f, 0)
        && DAT_0011d224 == 0.0f && DAT_0011ce44 == 0 ? '1' : 'x');

    reset(0.25f, 1.0f, 100, 200, 300);
    credits_fade_out();
    put(view_calls == 1 && view_is(0, 0, 0, 0)
        && target_calls == 0
        && present_calls == 1 && present_is(0, 0.0f, 0)
        && DAT_0011d224 == -0.75f && DAT_0011ce44 == 4 ? '2' : 'x');

    reset(0.5f, 0.0f, -101, -101, 7);
    credits_fade_out();
    put(view_calls == 1 && view_is(0, -50, -50, 3)
        && target_calls == 1 && target_value == -50
        && present_calls == 1 && present_is(0, 0.5f, 0)
        && DAT_0011d224 == 0.5f && DAT_0011ce44 == 0 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
