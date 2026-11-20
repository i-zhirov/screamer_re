/* The test harness for the rebuilt `sound_event_next` module.
 *
 * The harness holds the voice context, the source distance table, the voice
 * records, the four event records, the frame table, the three event pointers,
 * the render flag, and the setup stub.
 *
 * The harness checks the voice record pointer, the event pointer, the returned
 * value, the setup call and its basis argument, the render flag, and the three
 * source pointers.  The voice record base sits in the middle of the storage, so
 * a negative voice slot stays inside the storage.
 *
 * The harness runs five paths:
 *
 *     0  the index 0 with the voice slot 2 and a non-null event: the function
 *        returns 1 and sets every value
 *     1  the index 1 with the voice slot 0 and a null event: the function
 *        returns 0, runs no setup, and changes no output global
 *     2  the index 2 with the signed voice slot -1 and a negative event offset:
 *        the function extends the slot and adds the offset as a signed value
 *     3  the index 3 with the voice slot 5 and the render flag 0: the function
 *        still returns 1 for a non-null event
 *     4  two calls at one index: the first call returns 1 and the null second
 *        call returns 0 and clears the event pointer
 *     D  the exit marker
 */
#pragma off (check_stack)

#define VOICE_RECORD_SIZE 0x1f4
#define VOICE_BASIS_OFFSET 0x118
#define VOICE_SLOT_BASE 8
#define VOICE_SLOT_COUNT 16
#define EVENT_HEADER_SIZE 0x18
#define EVENT_COUNT 4
#define FRAME_BIAS 0x20
#define FRAME_STORAGE_SIZE 0x400
#define SENTINEL (-1)

struct source_distance {
    short id;
    short distance;
};

struct voice_context {
    char *frame_table;
    int voice_count;
    int frame_index;
    char *view_record;
    char *event_record;
};

struct sound_event {
    char header[EVENT_HEADER_SIZE];
    int render_flag;
    int source_offset;
    int point_offset;
    int screen_offset;
};

int sound_event_next(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct voice_context voice_context;
struct source_distance source_distances[VOICE_SLOT_COUNT];
char *voice_records;
int event_render_flag;
char *event_source;
char *event_point;
char *event_screen;

static char text[8];
static char voice_storage[VOICE_RECORD_SIZE * VOICE_SLOT_COUNT];
static char frame_storage[FRAME_STORAGE_SIZE];
static struct sound_event events[EVENT_COUNT];

static int setup_calls;
static char *setup_basis;

void sub_83560(char *basis)
{
    setup_calls = setup_calls + 1;
    setup_basis = basis;
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

static void clear_storage(void)
{
    int index;

    for (index = 0; index < VOICE_RECORD_SIZE * VOICE_SLOT_COUNT; index++) {
        voice_storage[index] = 0;
    }
    for (index = 0; index < FRAME_STORAGE_SIZE; index++) {
        frame_storage[index] = 0;
    }
}

static void reset(void)
{
    int index;

    clear_storage();
    for (index = 0; index < VOICE_SLOT_COUNT; index++) {
        source_distances[index].id = 0;
        source_distances[index].distance = 0;
    }
    for (index = 0; index < EVENT_COUNT; index++) {
        events[index].render_flag = 0;
        events[index].source_offset = 0;
        events[index].point_offset = 0;
        events[index].screen_offset = 0;
    }
    voice_records = voice_storage + VOICE_RECORD_SIZE * VOICE_SLOT_BASE;
    voice_context.frame_table = frame_storage + FRAME_BIAS;
    voice_context.voice_count = 0;
    voice_context.frame_index = 0;
    voice_context.view_record = (char *)SENTINEL;
    voice_context.event_record = (char *)SENTINEL;
    event_render_flag = SENTINEL;
    event_source = (char *)SENTINEL;
    event_point = (char *)SENTINEL;
    event_screen = (char *)SENTINEL;
    setup_calls = 0;
    setup_basis = 0;
}

static char *voice_at(short id)
{
    return voice_records + id * VOICE_RECORD_SIZE;
}

static int check_null(int index, short id)
{
    char *voice;
    int result;

    reset();
    voice_context.frame_index = index;
    source_distances[index].id = id;
    voice = voice_at(id);
    *(struct sound_event **)voice = 0;

    result = sound_event_next();
    return result == 0
        && voice_context.view_record == voice
        && voice_context.event_record == 0
        && setup_calls == 0
        && event_render_flag == SENTINEL
        && event_source == (char *)SENTINEL
        && event_point == (char *)SENTINEL
        && event_screen == (char *)SENTINEL;
}

static int check_event(int index, short id, int slot, int flag, int off1,
    int off2, int off3)
{
    char *voice;
    int result;

    reset();
    voice_context.frame_index = index;
    source_distances[index].id = id;
    voice = voice_at(id);
    *(struct sound_event **)voice = &events[slot];
    events[slot].render_flag = flag;
    events[slot].source_offset = off1;
    events[slot].point_offset = off2;
    events[slot].screen_offset = off3;

    result = sound_event_next();
    return result == 1
        && voice_context.view_record == voice
        && voice_context.event_record == (char *)&events[slot]
        && setup_calls == 1
        && setup_basis == voice + VOICE_BASIS_OFFSET
        && event_render_flag == flag
        && event_source == frame_storage + FRAME_BIAS + off1
        && event_point == frame_storage + FRAME_BIAS + off2
        && event_screen == frame_storage + FRAME_BIAS + off3;
}

static int check_two_calls(int index, short id, int slot)
{
    char *voice;
    int first;
    int second;

    reset();
    voice_context.frame_index = index;
    source_distances[index].id = id;
    voice = voice_at(id);
    *(struct sound_event **)voice = &events[slot];
    events[slot].render_flag = 0x1234;
    events[slot].source_offset = 0x10;
    events[slot].point_offset = 0x20;
    events[slot].screen_offset = 0x30;

    first = sound_event_next();
    *(struct sound_event **)voice = 0;
    second = sound_event_next();
    return first == 1
        && second == 0
        && voice_context.view_record == voice
        && voice_context.event_record == 0
        && setup_calls == 1;
}

int main(void)
{
    put(check_event(0, 2, 0, 0x2000, 0x10, 0x20, 0x30) ? '0' : 'x');
    put(check_null(1, 0) ? '1' : 'x');
    put(check_event(2, -1, 1, 0x4000, -0x10, 0, 0x7f) ? '2' : 'x');
    put(check_event(3, 5, 2, 0, 0x100, 0x200, 0x300) ? '3' : 'x');
    put(check_two_calls(4, 3, 3) ? '4' : 'x');
    put('D');
    quit();
    return 0;
}
