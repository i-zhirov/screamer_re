/* The test harness for the rebuilt `fade_update` module.
 *
 * The harness holds the state word, the rate, and the fade value.  The
 * `video_present` stub records the scale, the offset, and the call count.  The
 * harness runs seven paths:
 *
 *     0  state 0: nothing runs and the value stays
 *     1  state 1: the value ramps up and the state stays
 *     2  state 1: the value passes 1.0, clamps, and the state clears
 *     3  state 1: the value reaches exactly 1.0, clamps, and the state clears
 *     4  state 2: the value ramps down and the state stays
 *     5  state 2: the value passes 0.0, clamps, and the state clears
 *     6  state 2: the value reaches exactly 0.0 and the state stays
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void fade_update(float delta);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_0011fac8;
float DAT_0018e08;
float DAT_0011d224;

static char text[8];
static float got_scale;
static int got_offset;
static int present_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void video_present(float scale, int offset)
{
    got_scale = scale;
    got_offset = offset;
    present_calls = present_calls + 1;
}

static void reset(unsigned int state, float rate, float value)
{
    DAT_0011fac8 = state;
    DAT_0018e08 = rate;
    DAT_0011d224 = value;
    got_scale = -1.0f;
    got_offset = -1;
    present_calls = 0;
}

int main(void)
{
    reset(0, 2.0f, 0.5f);
    fade_update(0.25f);
    put(present_calls == 0 && DAT_0011d224 == 0.5f && DAT_0011fac8 == 0 ? '0' : 'x');

    reset(1, 2.0f, 0.0f);
    fade_update(0.25f);
    put(present_calls == 1 && DAT_0011d224 == 0.5f && DAT_0011fac8 == 1
        && got_scale == 0.5f && got_offset == 0 ? '1' : 'x');

    reset(1, 10.0f, 0.8f);
    fade_update(0.1f);
    put(present_calls == 1 && DAT_0011d224 == 1.0f && DAT_0011fac8 == 0
        && got_scale == 1.0f && got_offset == 0 ? '2' : 'x');

    reset(1, 2.0f, 0.5f);
    fade_update(0.25f);
    put(present_calls == 1 && DAT_0011d224 == 1.0f && DAT_0011fac8 == 0 ? '3' : 'x');

    reset(2, 2.0f, 1.0f);
    fade_update(0.25f);
    put(present_calls == 1 && DAT_0011d224 == 0.5f && DAT_0011fac8 == 2
        && got_scale == 0.5f && got_offset == 0 ? '4' : 'x');

    reset(2, 10.0f, 0.2f);
    fade_update(0.1f);
    put(present_calls == 1 && DAT_0011d224 == 0.0f && DAT_0011fac8 == 0
        && got_scale == 0.0f && got_offset == 0 ? '5' : 'x');

    reset(2, 2.0f, 0.5f);
    fade_update(0.25f);
    put(present_calls == 1 && DAT_0011d224 == 0.0f && DAT_0011fac8 == 2
        && got_scale == 0.0f && got_offset == 0 ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
