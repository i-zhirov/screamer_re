/* The test harness for the rebuilt `path_keyframe_append` module.
 *
 * The harness holds the view object, the recording state, and eight keyframe
 * records.  The state points at the keyframes and the view object holds the
 * position, the angle, and the other fields of the append.
 *
 * The harness runs six paths:
 *
 *     0  the active flag is zero: the function writes nothing
 *     1  the index is zero: the function writes the first keyframe and
 *        advances the index
 *     2  the index is two: the function writes the third keyframe and
 *        advances the index
 *     3  the index is the last slot: the function writes the keyframe, keeps
 *        the index, and clears the active flag
 *     4  the active flag is -1: the function writes nothing
 *     5  the count is one and the index is zero: the function keeps the index
 *        at zero and clears the active flag
 *     D  the exit marker
 *
 * The float values arrive as bit patterns, because the minimal image does not
 * load the constant segment.
 */
#pragma off (check_stack)

void path_keyframe_append(char *view, char *state);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float DAT_00118e10;

static char text[8];
static char view[0x800];
static char state[0x30];
static char frames[0x28 * 8];

static unsigned int float_bits(float value)
{
    union {
        float f;
        unsigned int u;
    } number;

    number.f = value;
    return number.u;
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

static void set_view(int base)
{
    *(int *)(view + 0x4b0) = base;
    *(int *)(view + 0x4b4) = base + 0x10;
    *(int *)(view + 0x4b8) = base + 0x20;
    *(int *)(view + 0x130) = 1;
    *(int *)(view + 0x134) = 2;
    *(int *)(view + 0x138) = 3;
    *(float *)(view + 0x4c8) = (float)base + 0.5f;
    *(float *)(view + 0x4cc) = (float)base + 1.5f;
    *(float *)(view + 0x4d0) = (float)base + 2.5f;
    *(float *)(view + 0x124) = 0.25f;
    *(float *)(view + 0x128) = 0.5f;
    *(float *)(view + 0x12c) = 0.75f;
    *(int *)(view + 0x7b4) = base + 0x40;
    *(int *)(view + 0x7b0) = base + 0x50;
    *(float *)(view + 0x680) = (float)base + 3.5f;
}

static void fill_frames(void)
{
    int offset;

    for (offset = 0; offset < 0x28 * 8; offset += 4) {
        *(unsigned int *)(frames + offset) = 0xdeadbeefu;
    }
}

static void reset(int index, int count, int active)
{
    int offset;

    for (offset = 0; offset < 0x800; offset++) {
        view[offset] = 0;
    }
    for (offset = 0; offset < 0x30; offset++) {
        state[offset] = 0;
    }
    fill_frames();
    *(int *)(state + 0x00) = count;
    *(int *)(state + 0x04) = index;
    *(int *)(state + 0x08) = 0x11223344;
    *(int *)(state + 0x0c) = active;
    *(char **)(state + 0x20) = frames;
    DAT_00118e10 = 123.5f;
}

static int frame_sentinel(int frame)
{
    int offset;
    char *p = frames + frame * 0x28;

    for (offset = 0; offset < 0x28; offset += 4) {
        if (*(unsigned int *)(p + offset) != 0xdeadbeefu) {
            return 0;
        }
    }
    return 1;
}

static int frame_ok(int frame)
{
    char *p = frames + frame * 0x28;

    return *(int *)(p + 0x00) == *(int *)(view + 0x4b0)
            + *(int *)(view + 0x130)
        && *(int *)(p + 0x04) == *(int *)(view + 0x4b4)
            + *(int *)(view + 0x134)
        && *(int *)(p + 0x08) == *(int *)(view + 0x4b8)
            + *(int *)(view + 0x138)
        && float_bits(*(float *)(p + 0x0c)) == float_bits(
            *(float *)(view + 0x4c8) + *(float *)(view + 0x124))
        && float_bits(*(float *)(p + 0x10)) == float_bits(
            *(float *)(view + 0x4cc) + *(float *)(view + 0x128))
        && float_bits(*(float *)(p + 0x14)) == float_bits(
            *(float *)(view + 0x4d0) + *(float *)(view + 0x12c))
        && *(int *)(p + 0x18) == *(int *)(view + 0x7b4)
        && *(int *)(p + 0x1c) == *(int *)(view + 0x7b0)
        && float_bits(*(float *)(p + 0x20)) == float_bits(
            *(float *)(view + 0x680))
        && float_bits(*(float *)(p + 0x24)) == float_bits(DAT_00118e10);
}

static int others_sentinel(int frame)
{
    int index;

    for (index = 0; index < 8; index++) {
        if (index != frame && !frame_sentinel(index)) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    reset(0, 4, 0);
    set_view(100);
    path_keyframe_append(view, state);
    put(*(int *)(state + 0x04) == 0
        && *(int *)(state + 0x08) == 0x11223344
        && *(int *)(state + 0x0c) == 0
        && others_sentinel(-1) ? '0' : 'x');

    reset(0, 4, 1);
    set_view(100);
    path_keyframe_append(view, state);
    put(*(int *)(state + 0x04) == 1
        && *(int *)(state + 0x08) == 1
        && *(int *)(state + 0x0c) == 1
        && frame_ok(0)
        && others_sentinel(0) ? '1' : 'x');

    reset(2, 4, 1);
    set_view(200);
    path_keyframe_append(view, state);
    put(*(int *)(state + 0x04) == 3
        && *(int *)(state + 0x08) == 3
        && *(int *)(state + 0x0c) == 1
        && frame_ok(2)
        && others_sentinel(2) ? '2' : 'x');

    reset(3, 4, 1);
    set_view(300);
    path_keyframe_append(view, state);
    put(*(int *)(state + 0x04) == 3
        && *(int *)(state + 0x08) == 3
        && *(int *)(state + 0x0c) == 0
        && frame_ok(3)
        && others_sentinel(3) ? '3' : 'x');

    reset(1, 4, -1);
    set_view(400);
    path_keyframe_append(view, state);
    put(*(int *)(state + 0x04) == 1
        && *(int *)(state + 0x08) == 0x11223344
        && *(int *)(state + 0x0c) == -1
        && others_sentinel(-1) ? '4' : 'x');

    reset(0, 1, 1);
    set_view(500);
    path_keyframe_append(view, state);
    put(*(int *)(state + 0x04) == 0
        && *(int *)(state + 0x08) == 0
        && *(int *)(state + 0x0c) == 0
        && frame_ok(0)
        && others_sentinel(0) ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
