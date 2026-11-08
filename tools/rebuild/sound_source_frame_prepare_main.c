/* The test harness for the rebuilt `sound_source_frame_prepare` module.
 *
 * The harness holds the frame table, the frame index, the voice record, the
 * context record, the source record, the sample offset, the camera record, and
 * the six callee stubs.
 *
 * The harness checks the source distance identifier, the frame table entry
 * pointer, the table base, the current object in the context and the voice
 * record, the six copied dwords, the four matrix composes with their three
 * pointers each, the frame select call, and the path choice of the three
 * helpers.
 *
 * The harness runs three paths:
 *
 *     0  the index 0 and a non-negative sample offset: the view helper and the
 *        pose copy run and the index transform does not
 *     1  the index 1 and a negative sample offset: the view helper and the
 *        pose copy do not run and the index transform runs
 *     2  the index 2: the frame table entry and the object follow the second
 *        table slot
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#define FRAME_TABLE_BIAS 0x18

struct source_distance {
    short id;
    short distance;
};

void sound_source_frame_prepare(char *context);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct source_distance source_distances[8];
int sound_sample_offset;
char *camera_state;

static char text[8];
static int table[0x30];
static char context[0x100];
static char voice[0x180];
static char source_record[0x40];
static int camera[0x80 / 4];
static char object_a[0x20];
static char object_b[0x20];
static char object_c[0x20];

static int flag_calls;
static int view_calls;
static int pose_calls;
static char *pose_voice;
static char *pose_table;
static int index_calls;
static int *index_values;

static int compose_calls;
static int *compose_source[4];
static int *compose_matrix[4];
static int *compose_destination[4];

static int select_calls;
static char *select_context;

void sub_8afb2(void)
{
    flag_calls = flag_calls + 1;
}

void sub_8afa0(void)
{
    view_calls = view_calls + 1;
}

void sub_8afc4(char *voice_pointer, char *table_pointer)
{
    pose_calls = pose_calls + 1;
    pose_voice = voice_pointer;
    pose_table = table_pointer;
}

void sub_8b610(int *values)
{
    index_calls = index_calls + 1;
    index_values = values;
}

void matrix_compose(int *source, int *matrix, int *destination)
{
    if (compose_calls < 4) {
        compose_source[compose_calls] = source;
        compose_matrix[compose_calls] = matrix;
        compose_destination[compose_calls] = destination;
    }
    compose_calls = compose_calls + 1;
}

void sound_source_frame_select(char *context_pointer)
{
    select_calls = select_calls + 1;
    select_context = context_pointer;
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

static int get(int offset)
{
    return *(int *)(context + offset);
}

static void set(int offset, int value)
{
    *(int *)(context + offset) = value;
}

static void reset(void)
{
    int position;

    for (position = 0; position < 0x100; position++) {
        context[position] = 0;
    }
    for (position = 0; position < 0x180; position++) {
        voice[position] = 0;
    }
    for (position = 0; position < 0x40; position++) {
        source_record[position] = 0;
    }
    for (position = 0; position < 8; position++) {
        source_distances[position].id = -1;
        source_distances[position].distance = -1;
    }
    for (position = 0; position < 0x20; position++) {
        table[position] = 0;
    }
    sound_sample_offset = 0;
    camera_state = (char *)camera;
    flag_calls = 0;
    view_calls = 0;
    pose_calls = 0;
    pose_voice = 0;
    pose_table = 0;
    index_calls = 0;
    index_values = 0;
    compose_calls = 0;
    select_calls = 0;
    select_context = 0;
}

static void set_entry(int index, int offset)
{
    *(int *)((char *)table + index * 4 + FRAME_TABLE_BIAS) = offset;
}

static void prepare(int index, char *object, int sample_offset)
{
    int position;

    reset();
    set(0x00, (int)table);
    set(0x08, index);
    set(0x0c, (int)voice);
    set(0xc8, (int)source_record);
    set_entry(index, (int)object - (int)table);
    sound_sample_offset = sample_offset;
    for (position = 0; position < 6; position++) {
        *(int *)(source_record + 0x08 + position * 4) =
            0x1100 + index * 0x100 + position;
    }
    sound_source_frame_prepare(context);
}

static int copy_ok(int index)
{
    int position;

    for (position = 0; position < 6; position++) {
        if (*(int *)(voice + 0x10 + position * 4)
            != 0x1100 + index * 0x100 + position) {
            return 0;
        }
    }
    return 1;
}

static int compose_ok(void)
{
    return compose_calls == 4
        && compose_source[0] == (int *)(context + 0x48)
        && compose_matrix[0] == (int *)(voice + 0x28)
        && compose_destination[0] == (int *)(voice + 0xa0)
        && compose_source[1] == (int *)(camera + 0x48 / 4)
        && compose_matrix[1] == (int *)(voice + 0xa0)
        && compose_destination[1] == (int *)(voice + 0x118)
        && compose_source[2] == (int *)(context + 0x78)
        && compose_matrix[2] == (int *)(camera + 0x18 / 4)
        && compose_destination[2] == (int *)(voice + 0xd0)
        && compose_source[3] == (int *)(voice + 0x58)
        && compose_matrix[3] == (int *)(voice + 0xd0)
        && compose_destination[3] == (int *)(voice + 0x148);
}

static int common_ok(int index, char *object)
{
    return source_distances[index].id == index
        && get(0xb4) == (int)((char *)table + index * 4 + FRAME_TABLE_BIAS)
        && get(0xb8) == (int)table
        && get(0x14) == (int)object
        && *(int *)voice == (int)object
        && copy_ok(index)
        && flag_calls == 1
        && compose_ok()
        && select_calls == 1
        && select_context == context;
}

int main(void)
{
    prepare(0, object_a, 0);
    put(common_ok(0, object_a)
        && view_calls == 1
        && pose_calls == 1
        && pose_voice == voice
        && pose_table == (char *)table
        && index_calls == 0
        ? '0' : 'x');

    prepare(1, object_b, -1);
    put(common_ok(1, object_b)
        && view_calls == 0
        && pose_calls == 0
        && index_calls == 1
        && index_values == (int *)(voice + 0x10)
        ? '1' : 'x');

    prepare(2, object_c, 0);
    put(common_ok(2, object_c)
        && view_calls == 1
        && pose_calls == 1
        && index_calls == 0
        ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
