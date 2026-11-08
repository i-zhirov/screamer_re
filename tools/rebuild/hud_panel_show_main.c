/* The test harness for the rebuilt `hud_panel_show` module.
 *
 * The harness holds the three 20-byte viewport records at `DAT_0015ccc`, the
 * state cells `DAT_0011ce54` and `DAT_0011ce40`, the filter cells
 * `DAT_0011ce4c` and `DAT_0011ce50`, and the sprite cells `DAT_0011cb14`
 * through `DAT_0011cb24`.  The `filter_update` stub follows the original at
 * `0x22b80` and records the target and the coefficient index.  The
 * `panel_update` stub records the flush.
 *
 * The harness runs six paths:
 *
 *     0  the state 0 centers the record 0 width 320 and the filter moves the
 *        position -320 toward 0
 *     1  the state 1 centers the record 1 width 200 and the filter moves the
 *        position 0 toward -320
 *     2  the state 0x12 with the flag set centers the record 2 width 320 and
 *        the filter moves the position 0 toward -340
 *     3  the state 5 centers the record 2 width 100 and the filter moves the
 *        position -400 toward 0 with the index 0x5a
 *     4  the state 5 keeps -400.5 and the sum -265.5 truncates toward zero to
 *        -265, so the truncating FPU stub is exercised
 *     5  the state 0x12 with the flag clear uses the record 2 and the index
 *        0x5a, not the -340 target
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void hud_panel_show(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char *DAT_0015ccc;
int DAT_001ce54;
int DAT_001ce40;
float DAT_001ce4c;
float DAT_001ce50;
int DAT_001cb14;
int DAT_001cb18;
int DAT_001cb1c;
int DAT_001cb20;
int DAT_001cb24;

static char text[8];
static unsigned char views[0x40];
static float coefficients[0x100];
static int filter_calls;
static float filter_target;
static int filter_index;
static int panel_calls;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_bits_float(float *value, unsigned int pattern)
{
    *(unsigned int *)value = pattern;
}

static void set_width(int index, int width)
{
    *(int *)(views + index * 0x14 + 8) = width;
}

void filter_update(float *value, float target, int index)
{
    filter_calls = filter_calls + 1;
    filter_target = target;
    filter_index = index;
    *value = target + (*value - target) * coefficients[index];
}

void panel_update(void)
{
    panel_calls = panel_calls + 1;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x40; index++) {
        views[index] = 0;
    }
    for (index = 0; index < 0x100; index++) {
        coefficients[index] = 0.5f;
    }
    DAT_0015ccc = views;
    DAT_001ce54 = 0;
    DAT_001ce40 = 0;
    DAT_001ce4c = 0.0f;
    DAT_001ce50 = 0.0f;
    DAT_001cb14 = 0;
    DAT_001cb18 = 0;
    DAT_001cb1c = 0;
    DAT_001cb20 = 0;
    DAT_001cb24 = 0;
    filter_calls = 0;
    filter_target = 0.0f;
    filter_index = 0;
    panel_calls = 0;
}

static int common(void)
{
    return filter_calls == 1
        && panel_calls == 1
        && DAT_001cb14 == 1
        && DAT_001cb24 == 0;
}

int main(void)
{
    int ok;

    reset();
    set_width(0, 320);
    set_bits_float(&DAT_001ce4c, 0xc3a00000);
    hud_panel_show();
    ok = common()
        && filter_index == 0x50
        && filter_target == 0.0f
        && DAT_001ce50 == 4.0f
        && DAT_001cb18 == -160
        && DAT_001cb1c == 8
        && DAT_001cb20 == 0;
    put(ok ? '0' : 'x');

    reset();
    set_width(1, 200);
    DAT_001ce54 = 1;
    DAT_001ce40 = 1;
    hud_panel_show();
    ok = common()
        && filter_index == 0x50
        && filter_target == -320.0f
        && DAT_001ce50 == 36.0f
        && DAT_001cb18 == -100
        && DAT_001cb1c == 72
        && DAT_001cb20 == 1;
    put(ok ? '1' : 'x');

    reset();
    set_width(2, 320);
    DAT_001ce54 = 0x12;
    DAT_001ce40 = 1;
    hud_panel_show();
    ok = common()
        && filter_index == 0x50
        && filter_target == -340.0f
        && DAT_001ce50 == 4.0f
        && DAT_001cb18 == -180
        && DAT_001cb1c == 8
        && DAT_001cb20 == 2;
    put(ok ? '2' : 'x');

    reset();
    set_width(2, 100);
    DAT_001ce54 = 5;
    set_bits_float(&DAT_001ce4c, 0xc3c80000);
    hud_panel_show();
    ok = common()
        && filter_index == 0x5a
        && filter_target == 0.0f
        && DAT_001ce50 == 4.0f
        && DAT_001cb18 == -130
        && DAT_001cb1c == 8
        && DAT_001cb20 == 2;
    put(ok ? '3' : 'x');

    reset();
    set_width(2, 100);
    DAT_001ce54 = 5;
    set_bits_float(&DAT_001ce4c, 0xc3c84000);
    coefficients[0x5a] = 1.0f;
    hud_panel_show();
    ok = common()
        && filter_index == 0x5a
        && DAT_001cb18 == -530
        && DAT_001cb20 == 2;
    put(ok ? '4' : 'x');

    reset();
    set_width(2, 100);
    DAT_001ce54 = 0x12;
    DAT_001ce40 = 0;
    hud_panel_show();
    ok = common()
        && filter_index == 0x5a
        && filter_target == 0.0f
        && DAT_001cb18 == 270
        && DAT_001cb20 == 2;
    put(ok ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
