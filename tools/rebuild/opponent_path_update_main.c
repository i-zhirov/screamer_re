/* The test harness for the rebuilt `opponent_path_update` module.
 *
 * The harness holds the path record, the state base, the node block, the curve
 * stubs, the speed value, the four constants, and the evaluation values.  The
 * arc sine, the square root, the absolute value, and the truncating conversion
 * live in the local FPU assembly file.  The harness runs five paths:
 *
 *     0  the phase stays below one: the target position and angle truncate
 *        toward zero, the history values copy, and the heading is pi/2
 *     1  the phase reaches one with a clear direction flag: the flag sets, the
 *        node holds, the 3-4-5 distance leaves the phase -3, and the heading
 *        wraps one turn to the stored result
 *     2  the phase reaches one with a set direction flag: the flag clears and
 *        the node index follows the next field at +0x10
 *     3  a negative new lateral value takes the 2.5 pi branch: the heading
 *        wraps to the stored result, and the direction and the node hold
 *     4  the phase is exactly one: the gate treats it as a segment end, and the
 *        distance leaves the phase -4.5
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.  The float comparisons use
 * the raw bits, because the constant pool of this toolchain reads at a low
 * address.
 */
#pragma off (check_stack)

void opponent_path_update(unsigned int state, void *body, float delta);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float DAT_00100ce1;
float DAT_00100ce5;
float DAT_00100ce9;
float DAT_00100ced;

static unsigned char path[0x100];
static unsigned char state_block[36];
static unsigned char node_block[3 * 56];
static char text[8];

static float speed_result;
static int speed_calls;
static void *speed_curve;
static float speed_arg;
static int eval_calls;
static unsigned int eval_state;
static void *eval_body;
static float eval_x;
static float eval_y;
static float eval_angle;
static float eval_next_x;
static float eval_next_y;
static float frame_step;

float opponent_curve_speed(void *curve, float phase)
{
    speed_calls = speed_calls + 1;
    speed_curve = curve;
    speed_arg = phase;
    return speed_result;
}

void opponent_curve_eval(unsigned int state, void *body)
{
    eval_calls = eval_calls + 1;
    eval_state = state;
    eval_body = body;
    *(float *)((char *)body + 0x60) = eval_x;
    *(float *)((char *)body + 0x64) = eval_y;
    *(float *)((char *)body + 0x68) = eval_angle;
    *(float *)((char *)body + 0x6c) = eval_next_x;
    *(float *)((char *)body + 0x70) = eval_next_y;
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

static void set_int(void *base, int offset, int value)
{
    *(int *)((char *)base + offset) = value;
}

static int get_int(void *base, int offset)
{
    return *(int *)((char *)base + offset);
}

static void set_bits(void *base, int offset, unsigned int bits)
{
    *(unsigned int *)((char *)base + offset) = bits;
}

static unsigned int get_bits(void *base, int offset)
{
    return *(unsigned int *)((char *)base + offset);
}

static void clear_bytes(void *base, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        ((char *)base)[index] = 0;
    }
}

static void reset(void)
{
    clear_bytes(path, 0x100);
    clear_bytes(node_block, 3 * 56);
    set_bits(&speed_result, 0, 0x3f800000);
    set_bits(&speed_arg, 0, 0);
    speed_calls = 0;
    speed_curve = 0;
    eval_calls = 0;
    eval_state = 0;
    eval_body = 0;
    set_bits(&eval_x, 0, 0);
    set_bits(&eval_y, 0, 0);
    set_bits(&eval_angle, 0, 0);
    set_bits(&eval_next_x, 0, 0);
    set_bits(&eval_next_y, 0, 0);
    set_bits(&frame_step, 0, 0);
    set_bits(&DAT_00100ce1, 0, 0x3fc90fdb);
    set_bits(&DAT_00100ce5, 0, 0x40fb53d2);
    set_bits(&DAT_00100ce9, 0, 0x40c90fdb);
    set_bits(&DAT_00100ced, 0, 0xc0c90fdb);
}

int main(void)
{
    reset();
    set_bits(path, 8, 0);
    set_bits(path, 0x1c, 0x3fa00000);
    set_bits(path, 0x24, 0x40200000);
    set_bits(path, 0x34, 0x41100000);
    set_bits(&eval_x, 0, 0x412c0000);
    set_bits(&eval_y, 0, 0xc1a20000);
    set_bits(&eval_angle, 0, 0xc0600000);
    set_bits(&eval_next_x, 0, 0);
    set_bits(&eval_next_y, 0, 0x3f000000);
    set_bits(&frame_step, 0, 0x3f000000);
    opponent_path_update((unsigned int)state_block, path, frame_step);
    put(get_int(path, 0x10) == 10 && get_int(path, 0x14) == -3
            && get_int(path, 0x18) == -20
            && get_bits(path, 0x28) == 0x3fa00000
            && get_bits(path, 0x30) == 0x40200000
            && get_bits(path, 0x1c) == 0
            && get_bits(path, 0x24) == 0x3f000000
            && get_bits(path, 0x38) == 0x41100000
            && get_bits(path, 0x34) == 0x3fc90fdb
            && get_bits(path, 8) == 0x3f000000
            && get_int(path, 0xc) == 0 && get_int(path, 4) == 0
            && speed_calls == 1 && eval_calls == 1
            && speed_curve == path + 0x3c
            && eval_state == (unsigned int)state_block
            && eval_body == path
        ? '0' : 'x');

    reset();
    set_bits(path, 8, 0);
    set_bits(path, 0xc, 0);
    set_bits(&eval_x, 0, 0x40400000);
    set_bits(&eval_y, 0, 0x40800000);
    set_bits(&frame_step, 0, 0x40000000);
    opponent_path_update((unsigned int)state_block, path, frame_step);
    put(get_int(path, 0xc) == 1 && get_int(path, 4) == 0
            && get_bits(path, 8) == 0xc0400000
            && get_int(path, 0x10) == 3 && get_int(path, 0x18) == 4
            && get_bits(path, 0x34) == 0x3fc90fdc
            && speed_calls == 2 && eval_calls == 2
        ? '1' : 'x');

    reset();
    set_bits(path, 8, 0);
    set_bits(path, 0xc, 1);
    set_bits(&eval_x, 0, 0x40400000);
    set_bits(&eval_y, 0, 0x40800000);
    set_int(state_block, 0, (int)node_block);
    set_int(node_block, 0x10, 3);
    set_bits(&frame_step, 0, 0x40000000);
    opponent_path_update((unsigned int)state_block, path, frame_step);
    put(get_int(path, 0xc) == 0 && get_int(path, 4) == 3
            && get_bits(path, 8) == 0xc0400000
            && get_int(path, 0x10) == 3 && get_int(path, 0x18) == 4
            && get_bits(path, 0x34) == 0x3fc90fdc
            && speed_calls == 2 && eval_calls == 2
        ? '2' : 'x');

    reset();
    set_bits(path, 8, 0);
    set_bits(path, 0xc, 1);
    set_int(path, 4, 7);
    set_bits(path, 0x1c, 0xbe800000);
    set_bits(path, 0x24, 0x40600000);
    set_bits(path, 0x34, 0x3f000000);
    set_bits(&eval_next_x, 0, 0);
    set_bits(&eval_next_y, 0, 0xbf800000);
    set_bits(&frame_step, 0, 0x3f000000);
    opponent_path_update((unsigned int)state_block, path, frame_step);
    put(get_int(path, 0xc) == 1 && get_int(path, 4) == 7
            && get_bits(path, 8) == 0x3f000000
            && get_bits(path, 0x28) == 0xbe800000
            && get_bits(path, 0x30) == 0x40600000
            && get_bits(path, 0x1c) == 0
            && get_bits(path, 0x24) == 0xbf800000
            && get_bits(path, 0x38) == 0x3f000000
            && get_bits(path, 0x34) == 0x3fc90fdc
            && speed_calls == 1 && eval_calls == 1
        ? '3' : 'x');

    reset();
    set_bits(path, 8, 0x3f000000);
    set_bits(path, 0xc, 0);
    set_bits(&eval_x, 0, 0x40400000);
    set_bits(&eval_y, 0, 0x40800000);
    set_bits(&frame_step, 0, 0x3f000000);
    opponent_path_update((unsigned int)state_block, path, frame_step);
    put(get_int(path, 0xc) == 1 && get_int(path, 4) == 0
            && get_bits(path, 8) == 0xc0900000
            && get_int(path, 0x10) == 3 && get_int(path, 0x18) == 4
            && get_bits(path, 0x34) == 0x3fc90fdc
            && speed_calls == 2 && eval_calls == 2
        ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
