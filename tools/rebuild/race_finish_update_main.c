/* The test harness for the rebuilt `race_finish_update` module.
 *
 * The harness holds the control bytes, the state timer, the frame delta, the
 * mode, the slot, the traffic count, the traffic records, the finish limit, the
 * effect queue, and the resource handle.  The seven overlay stubs record their
 * call count and their mode.  The panel stub records its four arguments.  The
 * queue stub returns the configured head.  The traffic stub records its count
 * and its base.  The release stub records its handle and returns a new handle.
 *
 * The harness runs fifteen paths:
 *
 *     0  the gate bit is clear: nothing runs
 *     1  the high finish bit draws the panel text and the timer stays below
 *     2  mode 1 draws its overlay
 *     3  mode 2 draws its overlay
 *     4  mode 4 draws its overlay
 *     5  mode 0x10 draws its overlay
 *     6  mode 0x20 draws its overlay
 *     7  mode 0x40 draws its overlay
 *     8  mode 3 draws nothing
 *     9  the timer passes the limit but the effect queue is not empty
 *     a  the finish path without the traffic mode sets the finish bits
 *     b  the traffic path marks the finished record and sets the finish bits
 *     c  the traffic path keeps the finished flag clear on an open record
 *     d  the high finish bit skips the traffic path and sets the finish bits
 *     e  the release runs on the short timer path
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void race_finish_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STRIDE 0x2a8
#define MAX_RECORDS 3

unsigned char DAT_00118e1c;
unsigned char DAT_00118e1d;
float DAT_00118e08;
float DAT_00118e28;
float race_finish_limit = 5.0f;
int DAT_0011d260;
int DAT_0010ffa4;
int DAT_00118fec;
int DAT_00115218;
int DAT_001100ec;
int DAT_00119080;
unsigned char DAT_00113cd8[STRIDE * MAX_RECORDS + 16];

static char text[8];
static int draw_calls;
static int draw_last;
static int panel_calls;
static int panel_text;
static int panel_y;
static int panel_font;
static int panel_color;
static int queue_calls;
static int traffic_calls;
static int traffic_count;
static int traffic_base_ok;
static int release_calls;
static int release_input;
static int release_result;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static unsigned int fbits(float value)
{
    return *(unsigned int *)&value;
}

static void set_int(int index, int offset, int value)
{
    *(int *)(DAT_00113cd8 + index * STRIDE + offset) = value;
}

static int get_int(int index, int offset)
{
    return *(int *)(DAT_00113cd8 + index * STRIDE + offset);
}

static void set_flag(int index, int offset, int value)
{
    *(unsigned char *)(DAT_00113cd8 + index * STRIDE + offset)
        = (unsigned char)value;
}

static void reset(void)
{
    int index;

    DAT_00118e1c = 0;
    DAT_00118e1d = 0;
    DAT_00118e08 = 0.0f;
    DAT_00118e28 = 0.0f;
    DAT_0011d260 = 0;
    DAT_0010ffa4 = 0;
    DAT_00118fec = 0;
    DAT_00115218 = 0;
    DAT_001100ec = -1;
    DAT_00119080 = -1;
    for (index = 0; index < STRIDE * MAX_RECORDS; index++) {
        DAT_00113cd8[index] = 0;
    }
    draw_calls = 0;
    draw_last = 0;
    panel_calls = 0;
    panel_text = 0;
    panel_y = 0;
    panel_font = 0;
    panel_color = 0;
    queue_calls = 0;
    traffic_calls = 0;
    traffic_count = 0;
    traffic_base_ok = 0;
    release_calls = 0;
    release_input = 0;
    release_result = 0;
}

void panel_draw_centered(int text_value, int y, int font, int color)
{
    panel_calls = panel_calls + 1;
    panel_text = text_value;
    panel_y = y;
    panel_font = font;
    panel_color = color;
}

void race_draw_1(void) { draw_calls = draw_calls + 1; draw_last = 1; }
void race_draw_2(void) { draw_calls = draw_calls + 1; draw_last = 2; }
void race_draw_4(void) { draw_calls = draw_calls + 1; draw_last = 4; }
void race_draw_16(void) { draw_calls = draw_calls + 1; draw_last = 16; }
void race_draw_32(void) { draw_calls = draw_calls + 1; draw_last = 32; }
void race_draw_64(void) { draw_calls = draw_calls + 1; draw_last = 64; }

int effect_queue_head(void)
{
    queue_calls = queue_calls + 1;
    return DAT_00119080;
}

void traffic_step(int count, char *base)
{
    traffic_calls = traffic_calls + 1;
    traffic_count = count;
    traffic_base_ok = base == (char *)DAT_00113cd8;
}

int resource_release(int handle)
{
    release_calls = release_calls + 1;
    release_input = handle;
    return release_result;
}

int main(void)
{
    race_finish_limit = 5.0f;
    reset();
    DAT_00118e1c = 0;
    DAT_00118e1d = 0;
    DAT_00118e28 = 1.0f;
    DAT_00118e08 = 1.0f;
    DAT_0011d260 = 1;
    DAT_00119080 = -1;
    DAT_001100ec = 7;
    release_result = 99;
    race_finish_update();
    put(draw_calls == 0
            && panel_calls == 0
            && release_calls == 0
            && DAT_001100ec == 7
            && fbits(DAT_00118e28) == 0x3f800000u
        ? '0' : 'x');

    reset();
    DAT_00118e1c = 4;
    DAT_00118e1d = 0x10;
    DAT_00118e28 = 0.0f;
    DAT_00118e08 = 0.5f;
    DAT_0011d260 = 1;
    DAT_00118fec = 0x1234;
    race_finish_update();
    put(panel_calls == 1
            && panel_text == 0x1234
            && panel_y == 0x5a
            && panel_font == 2
            && panel_color == 0
            && draw_calls == 0
            && queue_calls == 0
            && fbits(DAT_00118e28) == 0x3f000000u
        ? '1' : 'x');

    reset();
    DAT_00118e1c = 4;
    DAT_00118e08 = 0.5f;
    DAT_0011d260 = 1;
    DAT_00118e28 = 1.0f;
    race_finish_update();
    put(draw_calls == 1
            && draw_last == 1
            && panel_calls == 0
            && queue_calls == 0
            && fbits(DAT_00118e28) == 0x3fc00000u
        ? '2' : 'x');

    reset();
    DAT_00118e1c = 4;
    DAT_00118e08 = 0.5f;
    DAT_0011d260 = 2;
    DAT_00118e28 = 1.0f;
    race_finish_update();
    put(draw_calls == 1 && draw_last == 2 ? '3' : 'x');

    reset();
    DAT_00118e1c = 4;
    DAT_00118e08 = 0.5f;
    DAT_0011d260 = 4;
    DAT_00118e28 = 1.0f;
    race_finish_update();
    put(draw_calls == 1 && draw_last == 4 ? '4' : 'x');

    reset();
    DAT_00118e1c = 4;
    DAT_00118e08 = 0.5f;
    DAT_0011d260 = 0x10;
    DAT_00118e28 = 1.0f;
    DAT_00119080 = 0;
    race_finish_update();
    put(draw_calls == 1 && draw_last == 16 ? '5' : 'x');

    reset();
    DAT_00118e1c = 4;
    DAT_00118e08 = 0.5f;
    DAT_0011d260 = 0x20;
    DAT_00118e28 = 1.0f;
    DAT_00119080 = 0;
    race_finish_update();
    put(draw_calls == 1 && draw_last == 32 ? '6' : 'x');

    reset();
    DAT_00118e1c = 4;
    DAT_00118e08 = 0.5f;
    DAT_0011d260 = 0x40;
    DAT_00118e28 = 1.0f;
    DAT_00119080 = 0;
    race_finish_update();
    put(draw_calls == 1 && draw_last == 64 ? '7' : 'x');

    reset();
    DAT_00118e1c = 4;
    DAT_00118e08 = 0.5f;
    DAT_0011d260 = 3;
    DAT_00118e28 = 1.0f;
    race_finish_update();
    put(draw_calls == 0 && panel_calls == 0 ? '8' : 'x');

    reset();
    DAT_00118e1c = 4;
    DAT_00118e08 = 1.0f;
    DAT_0011d260 = 3;
    DAT_00118e28 = 4.5f;
    DAT_00119080 = 2;
    race_finish_update();
    put(fbits(DAT_00118e28) == 0x40b00000u
            && queue_calls == 1
            && traffic_calls == 0
            && release_calls == 0
            && DAT_00118e1d == 0
        ? '9' : 'x');

    reset();
    DAT_00118e1c = 4;
    DAT_00118e08 = 1.0f;
    DAT_0011d260 = 3;
    DAT_00118e28 = 4.5f;
    DAT_00119080 = -1;
    DAT_001100ec = 7;
    release_result = 0x55;
    race_finish_update();
    put(fbits(DAT_00118e28) == 0x40b00000u
            && queue_calls == 1
            && traffic_calls == 0
            && DAT_0010ffa4 == 0
            && DAT_00118e1d == 6
            && release_calls == 1
            && release_input == 7
            && DAT_001100ec == 0x55
        ? 'a' : 'x');

    reset();
    DAT_00118e1c = 4;
    DAT_00118e08 = 1.0f;
    DAT_0011d260 = 0x10;
    DAT_00118e28 = 4.5f;
    DAT_00119080 = -1;
    DAT_00115218 = 3;
    set_int(0, 0, -1);
    set_int(1, 0, 5);
    set_flag(1, 0x16, 2);
    set_int(2, 0, -2);
    race_finish_update();
    put(traffic_calls == 1
            && traffic_count == 3
            && traffic_base_ok == 1
            && DAT_0010ffa4 == 1
            && get_int(1, 0) == -3
            && DAT_00118e1d == 6
        ? 'b' : 'x');

    reset();
    DAT_00118e1c = 4;
    DAT_00118e08 = 1.0f;
    DAT_0011d260 = 0x10;
    DAT_00118e28 = 4.5f;
    DAT_00119080 = -1;
    DAT_00115218 = 2;
    set_int(0, 0, 5);
    set_flag(0, 0x16, 0);
    set_int(1, 0, 5);
    set_flag(1, 0x16, 2);
    race_finish_update();
    put(traffic_calls == 1
            && DAT_0010ffa4 == 1
            && get_int(0, 0) == 5
            && get_int(1, 0) == -3
            && DAT_00118e1d == 0
        ? 'c' : 'x');

    reset();
    DAT_00118e1c = 4;
    DAT_00118e1d = 0x10;
    DAT_00118e08 = 1.0f;
    DAT_0011d260 = 0x10;
    DAT_00118e28 = 4.5f;
    DAT_00119080 = -1;
    DAT_00115218 = 1;
    set_int(0, 0, 5);
    race_finish_update();
    put(panel_calls == 1
            && traffic_calls == 0
            && DAT_0010ffa4 == 0
            && DAT_00118e1d == 0x16
            && get_int(0, 0) == 5
        ? 'd' : 'x');

    reset();
    DAT_00118e1c = 4;
    DAT_00118e08 = 0.5f;
    DAT_0011d260 = 3;
    DAT_00118e28 = 1.0f;
    DAT_001100ec = 7;
    release_result = 0x22;
    race_finish_update();
    put(queue_calls == 0
            && release_calls == 1
            && release_input == 7
            && DAT_001100ec == 0x22
        ? 'e' : 'x');

    put('D');
    quit();
    return 0;
}
