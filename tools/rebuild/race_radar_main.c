/* The test harness for the rebuilt `race_radar` module.
 *
 * The harness holds the zoom counter, the mode byte, the traffic and car
 * record arrays, the ghost record, the player position and heading, the six
 * constant globals, and the four call stubs.  The stubs record their arguments
 * and the flush stub records the marker position at every flush.  The harness
 * runs nine paths:
 *
 *     0  the zoom counter is 0: the function returns at once
 *     1  the zoom counter is 1 and the mode bit 0x10 is set: the traffic
 *        record at `DAT_00113cd8` plots and the negative record is skipped
 *     2  the zoom counter is 2 and the mode bits 0 and 1 are set: the two car
 *        records at `DAT_00110ab8` plot
 *     3  the zoom counter is 3 and the mode bit 4 is set: the ghost record at
 *        `DAT_00118ee8` plots for the index `DAT_00118ed4 ^ 1`
 *     4  the mode bit 4 is set and `DAT_0011d264` is clear: no ghost plots
 *     5  the mode bit 4 is set and the ghost table entry is clear: no ghost
 *        plots
 *     6  the mode byte is clear: no record plots
 *     7  the mode holds the bits 0x10 and 1: only the traffic path plots
 *     8  the mode bit 0x10 is set and the traffic counter is 0: no plot
 *     D  the exit marker
 *
 * The constants are set from their exact bit patterns, so the harness does not
 * depend on a float literal or on the constant segment.
 */
#pragma off (check_stack)

void race_radar(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];

int DAT_0011d2a4;
unsigned char DAT_0011d260;
int DAT_0011d264;
int DAT_00115218;
unsigned char DAT_00113cd8[0x550];
int DAT_0010f8ec;
unsigned char DAT_00110ab8[0xb20];
int DAT_00118ed4;
int DAT_00118e98[0x48];
int DAT_00118ee8;
int DAT_00118ef0;
int DAT_0011fdcc;
int DAT_0011fdd4;
float DAT_0011fde8;

unsigned char DAT_0000055b;
int DAT_0000056c;
int DAT_00000570;
int DAT_00000574;
int DAT_00000578;
int DAT_00115ce4;
int DAT_00115ce8;
int DAT_00115cec;
int DAT_00115cf0;
int DAT_0011cb18;
int DAT_0011cb1c;
int DAT_0011cb20;

float radar_angle_scale;
float radar_pi;
float radar_heading_scale;
float radar_center_x;
float radar_center_y;
float radar_scale;

static int fill_calls;
static int fill_x;
static int fill_y;
static int fill_w;
static int fill_h;
static int render_calls;
static int render_x;
static int render_y;
static int render_angle;
static int rotate_calls;
static int rotate_in0;
static int rotate_in2;
static float rotate_angle;
static int rotate_out0;
static int rotate_out2;
static int flush_calls;
static int flush_x[8];
static int flush_y[8];
static int flush_mode[8];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_global_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *p = v.f;
}

static int float_bits_are(float value, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.f = value;
    return v.u == bits;
}

static void set_int_at(unsigned char *base, int offset, int value)
{
    *(int *)(base + offset) = value;
}

static void reset(void)
{
    int i;

    for (i = 0; i < 0x550; i++) {
        DAT_00113cd8[i] = 0;
    }
    for (i = 0; i < 0xb20; i++) {
        DAT_00110ab8[i] = 0;
    }
    for (i = 0; i < 0x48; i++) {
        DAT_00118e98[i] = 0;
    }
    DAT_0011d2a4 = 0;
    DAT_0011d260 = 0;
    DAT_0011d264 = 0;
    DAT_00115218 = 0;
    DAT_0010f8ec = 0;
    DAT_00118ed4 = 0;
    DAT_00118ee8 = 0;
    DAT_00118ef0 = 0;
    DAT_0011fdcc = 0;
    DAT_0011fdd4 = 0;
    DAT_0011fde8 = 0.0f;
    DAT_0000055b = 0;
    DAT_0000056c = 0;
    DAT_00000570 = 0;
    DAT_00000574 = 0;
    DAT_00000578 = 0;
    DAT_00115ce4 = 0;
    DAT_00115ce8 = 0;
    DAT_00115cec = 0;
    DAT_00115cf0 = 0;
    DAT_0011cb18 = 0;
    DAT_0011cb1c = 0;
    DAT_0011cb20 = 0;
    fill_calls = 0;
    fill_x = 0;
    fill_y = 0;
    fill_w = 0;
    fill_h = 0;
    render_calls = 0;
    render_x = 0;
    render_y = 0;
    render_angle = 0;
    rotate_calls = 0;
    rotate_in0 = 0;
    rotate_in2 = 0;
    rotate_angle = 0.0f;
    rotate_out0 = 0;
    rotate_out2 = 0;
    flush_calls = 0;
    set_global_float(&radar_angle_scale, 0x44800000);
    set_global_float(&radar_pi, 0x40490fdb);
    set_global_float(&radar_heading_scale, 0x37800000);
    set_global_float(&radar_center_x, 0x421c0000);
    set_global_float(&radar_center_y, 0x431f0000);
    set_global_float(&radar_scale, 0x40000000);
}

void map_fill(int x, int y, int width, int height)
{
    fill_calls = fill_calls + 1;
    fill_x = x;
    fill_y = y;
    fill_w = width;
    fill_h = height;
}

void world_render(int x, int y, int angle)
{
    render_calls = render_calls + 1;
    render_x = x;
    render_y = y;
    render_angle = angle;
}

void vector_rotate(int *out, int *in, float angle)
{
    rotate_calls = rotate_calls + 1;
    rotate_in0 = in[0];
    rotate_in2 = in[2];
    rotate_angle = angle;
    out[0] = rotate_out0;
    out[2] = rotate_out2;
}

void sprite_flush(void)
{
    if (flush_calls < 8) {
        flush_x[flush_calls] = DAT_0011cb18;
        flush_y[flush_calls] = DAT_0011cb1c;
        flush_mode[flush_calls] = DAT_0011cb20;
    }
    flush_calls = flush_calls + 1;
}

int main(void)
{
    reset();
    DAT_0011d2a4 = 0;
    race_radar();
    put(fill_calls == 0 && render_calls == 0 && rotate_calls == 0
        && flush_calls == 0 && DAT_0011cb18 == 0 && DAT_0011cb1c == 0
        && DAT_0011cb20 == 0 && DAT_0000055b == 0 ? '0' : 'x');

    reset();
    DAT_0011d2a4 = 1;
    DAT_0011d260 = 0x10;
    DAT_0011fdcc = 1000;
    DAT_0011fdd4 = 2000;
    set_global_float(&DAT_0011fde8, 0x00000000);
    DAT_00115218 = 2;
    set_int_at(DAT_00113cd8, 0, 0);
    set_int_at(DAT_00113cd8, 0x3c, 1100);
    set_int_at(DAT_00113cd8, 0x44, 1800);
    set_int_at(DAT_00113cd8, 0x2a8, -1);
    rotate_out0 = 10;
    rotate_out2 = -20;
    race_radar();
    put(fill_calls == 1 && fill_x == 8 && fill_y == 0xf8
        && fill_w == 0x90 && fill_h == 0x90
        && render_calls == 1 && render_x == 1000 && render_y == 2000
        && render_angle == 0
        && rotate_calls == 1 && rotate_in0 == 100 && rotate_in2 == -200
        && float_bits_are(rotate_angle, 0x00000000)
        && flush_calls == 2 && flush_x[0] == 77 && flush_y[0] == 317
        && flush_mode[0] == 5 && flush_x[1] == 0x4c && flush_y[1] == 0x13c
        && flush_mode[1] == 4
        && DAT_00115ce4 == 0 && DAT_00115ce8 == 0x280
        && DAT_00115cec == 0 && DAT_00115cf0 == 0x190
        && DAT_0000055b == 0xff && DAT_0000056c == 8
        && DAT_00000570 == 0x98 && DAT_00000574 == 0xf8
        && DAT_00000578 == 0x188 ? '1' : 'x');

    reset();
    DAT_0011d2a4 = 2;
    DAT_0011d260 = 1;
    DAT_0010f8ec = 2;
    set_int_at(DAT_00110ab8, 0x498, 100);
    set_int_at(DAT_00110ab8, 0x4a0, -200);
    set_int_at(DAT_00110ab8, 0x590 + 0x498, -300);
    set_int_at(DAT_00110ab8, 0x590 + 0x4a0, 400);
    rotate_out0 = 10;
    rotate_out2 = -20;
    race_radar();
    put(fill_calls == 1 && render_calls == 1 && render_angle == 0
        && rotate_calls == 2 && rotate_in0 == -300 && rotate_in2 == 400
        && flush_calls == 3 && flush_x[0] == 77 && flush_y[0] == 317
        && flush_mode[0] == 5 && flush_x[1] == 77 && flush_y[1] == 317
        && flush_mode[1] == 5 && flush_x[2] == 0x4c && flush_y[2] == 0x13c
        && flush_mode[2] == 4 ? '2' : 'x');

    reset();
    DAT_0011d2a4 = 3;
    DAT_0011d260 = 4;
    DAT_0011d264 = 1;
    DAT_00118ed4 = 0;
    DAT_00118e98[9] = 1;
    DAT_00118ee8 = 111;
    DAT_00118ef0 = 222;
    DAT_0011fdcc = 1000;
    DAT_0011fdd4 = 2000;
    rotate_out0 = 10;
    rotate_out2 = -20;
    race_radar();
    put(fill_calls == 1 && render_calls == 1
        && rotate_calls == 1 && rotate_in0 == -889 && rotate_in2 == -1778
        && flush_calls == 2 && flush_x[0] == 77 && flush_y[0] == 317
        && flush_mode[0] == 5 && flush_x[1] == 0x4c && flush_y[1] == 0x13c
        && flush_mode[1] == 4 ? '3' : 'x');

    reset();
    DAT_0011d2a4 = 1;
    DAT_0011d260 = 4;
    DAT_0011d264 = 0;
    race_radar();
    put(fill_calls == 1 && render_calls == 1 && rotate_calls == 0
        && flush_calls == 1 && flush_x[0] == 0x4c && flush_y[0] == 0x13c
        && flush_mode[0] == 4 ? '4' : 'x');

    reset();
    DAT_0011d2a4 = 1;
    DAT_0011d260 = 4;
    DAT_0011d264 = 1;
    DAT_00118ed4 = 0;
    DAT_00118e98[9] = 0;
    race_radar();
    put(fill_calls == 1 && render_calls == 1 && rotate_calls == 0
        && flush_calls == 1 && flush_x[0] == 0x4c && flush_y[0] == 0x13c
        && flush_mode[0] == 4 ? '5' : 'x');

    reset();
    DAT_0011d2a4 = 1;
    DAT_0011d260 = 0;
    race_radar();
    put(fill_calls == 1 && render_calls == 1 && rotate_calls == 0
        && flush_calls == 1 && flush_x[0] == 0x4c && flush_y[0] == 0x13c
        && flush_mode[0] == 4 ? '6' : 'x');

    reset();
    DAT_0011d2a4 = 2;
    DAT_0011d260 = 0x11;
    DAT_00115218 = 1;
    DAT_0010f8ec = 1;
    set_int_at(DAT_00113cd8, 0, 0);
    set_int_at(DAT_00113cd8, 0x3c, 100);
    set_int_at(DAT_00113cd8, 0x44, -200);
    set_int_at(DAT_00110ab8, 0x498, 300);
    set_int_at(DAT_00110ab8, 0x4a0, 400);
    rotate_out0 = 10;
    rotate_out2 = -20;
    race_radar();
    put(fill_calls == 1 && render_calls == 1 && rotate_calls == 1
        && rotate_in0 == 100 && rotate_in2 == -200
        && flush_calls == 2 && flush_x[0] == 77 && flush_y[0] == 317
        && flush_mode[0] == 5 && flush_x[1] == 0x4c && flush_y[1] == 0x13c
        && flush_mode[1] == 4 ? '7' : 'x');

    reset();
    DAT_0011d2a4 = 1;
    DAT_0011d260 = 0x10;
    DAT_00115218 = 0;
    race_radar();
    put(fill_calls == 1 && render_calls == 1 && rotate_calls == 0
        && flush_calls == 1 && flush_x[0] == 0x4c && flush_y[0] == 0x13c
        && flush_mode[0] == 4 ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
