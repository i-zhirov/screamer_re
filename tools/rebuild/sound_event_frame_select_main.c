/* The test harness for the rebuilt `sound_event_frame_select` module.
 *
 * The harness holds one event record.  The record holds the 16-entry frame
 * offset table at +0x28.  The harness runs ten paths:
 *
 *     0  the x 10, y 10, z 20 case selects the index 0
 *     1  the x 20, y 10, z 10 case selects the index 1
 *     2  the x 20, y -1, z -10 case selects the index 10
 *     3  the x 10, y -1, z -20 case selects the index 11
 *     4  the x -10, y 10, z -20 case selects the index 4
 *     5  the x -20, y 10, z -10 case selects the index 5
 *     6  the x -20, y 10, z 10 case selects the index 6
 *     7  the x -10, y 10, z 20 case selects the index 7
 *     8  the x 10, y 0, z 10 case selects the index 1 at the z == x boundary
 *     9  the x -10, y -1, z -10 case selects the index 13 at the -z == -x
 *        boundary with the negative y base
 *     D  the exit marker
 *
 * Each path checks the direction index in the global and the returned pointer
 * at the event base plus the selected offset.  Each path also checks that the
 * record header keeps its sentinel byte.
 */
#pragma off (check_stack)

#define EVENT_OFFSET_TABLE 0x28
#define EVENT_OFFSET_COUNT 16
#define HEADER_SENTINEL 0x5a

struct sound_event {
    char header[EVENT_OFFSET_TABLE];
    int frame_offset[EVENT_OFFSET_COUNT];
};

short *sound_event_frame_select(int x, int y, int z);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct sound_event *sound_event_record;
int frame_select_index;

static char text[8];
static struct sound_event event;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(void)
{
    int index;

    for (index = 0; index < EVENT_OFFSET_TABLE; index++) {
        event.header[index] = HEADER_SENTINEL;
    }
    for (index = 0; index < EVENT_OFFSET_COUNT; index++) {
        event.frame_offset[index] = index * 8;
    }
    sound_event_record = &event;
    frame_select_index = -1;
}

static int check(int x, int y, int z, int expected)
{
    short *frame;

    frame = sound_event_frame_select(x, y, z);
    return frame_select_index == expected
        && frame == (short *)((char *)&event + event.frame_offset[expected])
        && event.header[0] == HEADER_SENTINEL;
}

int main(void)
{
    reset();
    put(check(10, 10, 20, 0) ? '0' : 'x');

    reset();
    put(check(20, 10, 10, 1) ? '1' : 'x');

    reset();
    put(check(20, -1, -10, 10) ? '2' : 'x');

    reset();
    put(check(10, -1, -20, 11) ? '3' : 'x');

    reset();
    put(check(-10, 10, -20, 4) ? '4' : 'x');

    reset();
    put(check(-20, 10, -10, 5) ? '5' : 'x');

    reset();
    put(check(-20, 10, 10, 6) ? '6' : 'x');

    reset();
    put(check(-10, 10, 20, 7) ? '7' : 'x');

    reset();
    put(check(10, 0, 10, 1) ? '8' : 'x');

    reset();
    put(check(-10, -1, -10, 13) ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
