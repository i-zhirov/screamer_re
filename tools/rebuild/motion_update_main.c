/* The test harness for the rebuilt `motion_update` module.
 *
 * The harness holds the time base, the frame value, and the three raw position
 * values.  The `view_move` stub records its arguments and the `video_present`
 * stub records its arguments.  The harness runs three paths:
 *
 *     0  with the time base below zero: nothing runs and the value stays
 *     1  with the time base inside the range: the scaled values run, the time
 *        base advances, and the wrap stays clear
 *     2  with the advance reaching 1.0: the scaled values run, the time base
 *        wraps to -1.0, and the raw values run again
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void motion_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float DAT_00118e38;
float DAT_00118e08;
float DAT_00100d51;
int DAT_0011d2c0;
int DAT_0011d2bc;
int DAT_0011d2c4;

static char text[8];
static int view_calls;
static int present_calls;
static int view_a[2];
static int view_b[2];
static int view_c[2];
static float present_scale[2];
static int present_offset[2];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void view_move(int a, int b, int c)
{
    if (view_calls < 2) {
        view_a[view_calls] = a;
        view_b[view_calls] = b;
        view_c[view_calls] = c;
    }
    view_calls = view_calls + 1;
}

void video_present(float scale, int offset)
{
    if (present_calls < 2) {
        present_scale[present_calls] = scale;
        present_offset[present_calls] = offset;
    }
    present_calls = present_calls + 1;
}

static void reset(float time, float frame, int a, int b, int c)
{
    DAT_00118e38 = time;
    DAT_00118e08 = frame;
    DAT_00100d51 = 2.0f;
    DAT_0011d2c0 = a;
    DAT_0011d2bc = b;
    DAT_0011d2c4 = c;
    view_calls = 0;
    present_calls = 0;
}

int main(void)
{
    reset(-1.0f, 0.125f, 100, 200, 300);
    motion_update();
    put(view_calls == 0 && present_calls == 0 && DAT_00118e38 == -1.0f ? '0' : 'x');

    reset(0.25f, 0.125f, 100, 200, 300);
    motion_update();
    put(view_calls == 1 && present_calls == 1
        && view_a[0] == 25 && view_b[0] == 50 && view_c[0] == 75
        && present_scale[0] == 0.25f && present_offset[0] == 0
        && DAT_00118e38 == 0.5f ? '1' : 'x');

    reset(0.5f, 0.5f, 100, 200, 300);
    motion_update();
    put(view_calls == 2 && present_calls == 2
        && view_a[0] == 50 && view_b[0] == 100 && view_c[0] == 150
        && view_a[1] == 100 && view_b[1] == 200 && view_c[1] == 300
        && present_scale[0] == 0.5f && present_offset[0] == 0
        && present_scale[1] == 1.0f && present_offset[1] == 0
        && DAT_00118e38 == -1.0f ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
