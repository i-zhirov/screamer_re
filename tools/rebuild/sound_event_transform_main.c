/* The test harness for the rebuilt `sound_event_transform` module.
 *
 * The harness holds the source list, the point list, the ES destination, the
 * projection table, the screen bias, and the twelve basis values.  The module
 * writes the camera-space points and the packed screen coordinates.
 *
 * The harness runs these paths:
 *
 *     0  the identity basis and the point 16, 32, 48: the positive third value
 *        stores the clipped point 0x10001000
 *     1  the point 5, 7, 0: the zero third value selects the projection table at
 *        the index 0
 *     2  the point -16, -32, -48: the negative values use the table at the
 *        index -48 and the arithmetic shift
 *     3  two points: the 6-byte source stride, the 12-byte point stride, and the
 *        6-byte screen stride
 *     4  the off-diagonal basis and the three translations
 *     5  the third value 1 and the third value -1: the two sides of the compare
 *     6  the end marker at the first record: the function writes no value
 *     D  the exit marker
 *
 * The harness fills the point list and the screen with a sentinel.  It checks
 * the exact output values and the sentinel bytes between the screen writes, so
 * the write bounds are exact.
 */
#pragma off (check_stack)

#define POINT_SENTINEL 0x5a5a5a5a
#define SCREEN_SENTINEL 0xa5
#define SCREEN_BYTES 128
#define PROJECTION_PAD 64
#define PROJECTION_SIZE 160

void sound_event_transform(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

short *event_source;
int *event_points;
char *event_screen;
int *event_projection;
int DAT_0007a7dc;
int event_matrix[12];

static char text[8];
static short source_buffer[64];
static int point_buffer[32];
static unsigned char screen_buffer[SCREEN_BYTES];
static int projection_storage[PROJECTION_SIZE];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void clear_shorts(short *p, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        p[index] = 0x7fff;
    }
}

static void clear_ints(int *p, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        p[index] = 0;
    }
}

static void clear_bytes(unsigned char *p, int count, unsigned char value)
{
    int index;

    for (index = 0; index < count; index++) {
        p[index] = value;
    }
}

static void reset(void)
{
    int index;

    clear_shorts(source_buffer, 64);
    for (index = 0; index < 32; index++) {
        point_buffer[index] = POINT_SENTINEL;
    }
    clear_bytes(screen_buffer, SCREEN_BYTES, SCREEN_SENTINEL);
    clear_ints(projection_storage, PROJECTION_SIZE);
    clear_ints(event_matrix, 12);
    event_source = source_buffer;
    event_points = point_buffer;
    event_screen = (char *)screen_buffer;
    event_projection = &projection_storage[PROJECTION_PAD];
    DAT_0007a7dc = 0;
}

static void set_identity(void)
{
    event_matrix[0] = 0x4000;
    event_matrix[4] = 0x4000;
    event_matrix[8] = 0x4000;
}

static void set_off_diagonal(void)
{
    event_matrix[0] = 0x2000;
    event_matrix[1] = 0x1000;
    event_matrix[4] = 0x4000;
    event_matrix[5] = 0x2000;
    event_matrix[8] = 0x4000;
    event_matrix[9] = 10;
    event_matrix[10] = -20;
}

static void set_point(int record, int x, int y, int z)
{
    source_buffer[record * 3 + 0] = (short)x;
    source_buffer[record * 3 + 1] = (short)y;
    source_buffer[record * 3 + 2] = (short)z;
}

static int point_is(int index, int value)
{
    return point_buffer[index] == value;
}

static int screen_is(int offset, int value)
{
    return *(int *)(screen_buffer + offset) == value;
}

static int byte_is(int offset, int value)
{
    return screen_buffer[offset] == (unsigned char)value;
}

int main(void)
{
    reset();
    set_identity();
    set_point(0, 16, 32, 48);
    sound_event_transform();
    put(point_is(0, 16)
        && point_is(1, 32)
        && point_is(2, 48)
        && point_is(3, POINT_SENTINEL)
        && screen_is(0, 0x10001000)
        && byte_is(4, SCREEN_SENTINEL)
        && byte_is(5, SCREEN_SENTINEL) ? '0' : 'x');

    reset();
    set_identity();
    set_point(0, 5, 7, 0);
    event_projection[0] = -65536;
    DAT_0007a7dc = 100;
    sound_event_transform();
    put(point_is(0, 5)
        && point_is(1, 7)
        && point_is(2, 0)
        && screen_is(0, 0x01350056) ? '1' : 'x');

    reset();
    set_identity();
    set_point(0, -16, -32, -48);
    event_projection[-48] = 0x10000;
    sound_event_transform();
    put(point_is(0, -16)
        && point_is(1, -32)
        && point_is(2, -48)
        && screen_is(0, 0x011fffc0) ? '2' : 'x');

    reset();
    set_identity();
    set_point(0, 1, 2, 3);
    set_point(1, 4, 5, 0);
    set_point(2, 0x7fff, 0, 0);
    event_projection[0] = -65536;
    DAT_0007a7dc = 100;
    sound_event_transform();
    put(point_is(0, 1)
        && point_is(1, 2)
        && point_is(2, 3)
        && point_is(3, 4)
        && point_is(4, 5)
        && point_is(5, 0)
        && point_is(6, POINT_SENTINEL)
        && screen_is(0, 0x10001000)
        && screen_is(6, 0x0137005a)
        && byte_is(4, SCREEN_SENTINEL)
        && byte_is(5, SCREEN_SENTINEL)
        && byte_is(10, SCREEN_SENTINEL)
        && byte_is(11, SCREEN_SENTINEL) ? '3' : 'x');

    reset();
    set_off_diagonal();
    set_point(0, 8, 16, 20);
    sound_event_transform();
    put(point_is(0, 18)
        && point_is(1, 6)
        && point_is(2, 20)
        && screen_is(0, 0x10001000) ? '4' : 'x');

    reset();
    set_identity();
    set_point(0, 0, 0, 1);
    set_point(1, 0, 0, -1);
    set_point(2, 0x7fff, 0, 0);
    event_projection[-1] = -65536;
    DAT_0007a7dc = 0x200;
    sound_event_transform();
    put(point_is(0, 0)
        && point_is(1, 0)
        && point_is(2, 1)
        && point_is(3, 0)
        && point_is(4, 0)
        && point_is(5, -1)
        && screen_is(0, 0x10001000)
        && screen_is(6, 0x013f0200) ? '5' : 'x');

    reset();
    set_identity();
    sound_event_transform();
    put(point_is(0, POINT_SENTINEL)
        && byte_is(0, SCREEN_SENTINEL) ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
