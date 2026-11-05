/* The test harness for the rebuilt `hud_update` module.
 *
 * The harness holds the session state and the three overlay stubs.  Each stub
 * prints its marker.  The harness runs two states and checks the call set:
 *
 *     4  the state 4: the first two overlays run, the third does not
 *     2  the state 2: the third overlay runs, the first two do not
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void hud_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_0011ce44;

static char text[8];
static int overlay_0_calls;
static int overlay_1_calls;
static int overlay_2_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void hud_overlay_0(void)
{
    overlay_0_calls = overlay_0_calls + 1;
}

void hud_overlay_1(void)
{
    overlay_1_calls = overlay_1_calls + 1;
}

void hud_overlay_2(void)
{
    overlay_2_calls = overlay_2_calls + 1;
}

int main(void)
{
    DAT_0011ce44 = 4;
    overlay_0_calls = 0;
    overlay_1_calls = 0;
    overlay_2_calls = 0;
    hud_update();
    put(overlay_0_calls == 1 && overlay_1_calls == 1 && overlay_2_calls == 0 ? '0' : 'x');
    DAT_0011ce44 = 3;
    overlay_0_calls = 0;
    overlay_1_calls = 0;
    overlay_2_calls = 0;
    hud_update();
    put(overlay_0_calls == 0 && overlay_1_calls == 0 && overlay_2_calls == 1 ? '2' : 'x');
    put('D');
    quit();
    return 0;
}
