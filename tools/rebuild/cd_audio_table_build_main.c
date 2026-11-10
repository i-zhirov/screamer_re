/* The test harness for the rebuilt `cd_audio_table_build` module.
 *
 * The harness holds the state flag, the first and last track cells, the MSCDEX
 * request block, the track table, and the stubs of the three request helpers
 * and the time helper.  The stubs fill the request block, return the configured
 * status word, and record the track order.
 *
 * The track n carries the minute n, the second 0, and the frame 0, so its start
 * time is 75 * (60 * n) - 150.  The lead-out carries the minute 200, so its time
 * is 75 * (60 * 200) - 150.
 *
 * The paths:
 *
 *     0  the state flag is set: no request and the flag stays set
 *     1  three failed status words: the function returns 1 and sets the flag
 *     2  one failed status word then a success, all data: return 1, flag set
 *     3  the first track is audio: the two-track table and return 0
 *     4  the last track 0x20 is clamped to 0x14, all data: return 1, flag set
 *     5  the first two tracks data and the third audio: the table from the third
 *     6  the only track is audio: the one-track table
 *     7  the track 19 is audio and the last 0x20 is clamped: the two-track
 *        table from 19
 *     D  the exit marker
 */
#pragma off (check_stack)

int cd_audio_table_build(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00166688;
int DAT_0016668e;
int DAT_00166692;
unsigned int DAT_001666a4;
int DAT_001666c0[64];

static char text[8];
static unsigned char request[0x80];
static int status_words[8];
static int status_count;
static int status_index;
static unsigned char track_control[256];
static int track_calls[64];
static int track_count;
static int calls_260a0;
static int calls_26120;
static int first_track;
static int last_track_raw;
static int leadout_minute;
static int leadout_second;
static int leadout_frame;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static unsigned char *request_fields(void)
{
    return (unsigned char *)DAT_001666a4 + 0x40;
}

void sub_260a0(void)
{
    calls_260a0 = calls_260a0 + 1;
}

int sub_26120(void)
{
    unsigned char *fields;
    int status;

    calls_26120 = calls_26120 + 1;
    fields = request_fields();
    fields[1] = (unsigned char)first_track;
    fields[2] = (unsigned char)last_track_raw;
    fields[3] = (unsigned char)leadout_frame;
    fields[4] = (unsigned char)leadout_second;
    fields[5] = (unsigned char)leadout_minute;
    status = status_words[status_index];
    if (status_index < status_count - 1) {
        status_index = status_index + 1;
    }
    return status;
}

void sub_261a8(int track)
{
    unsigned char *fields;

    fields = request_fields();
    track_calls[track_count] = track;
    track_count = track_count + 1;
    fields[2] = 0;
    fields[3] = 0;
    fields[4] = (unsigned char)track;
    fields[6] = track_control[track];
}

int sub_2626c(int minute, int second, int frame)
{
    return 75 * (60 * minute + second) + frame - 150;
}

static void reset(void)
{
    int index;

    DAT_00166688 = 0;
    DAT_0016668e = 0x22222222;
    DAT_00166692 = 0x33333333;
    DAT_001666a4 = (unsigned int)request;
    status_count = 0;
    status_index = 0;
    track_count = 0;
    calls_260a0 = 0;
    calls_26120 = 0;
    first_track = 1;
    last_track_raw = 1;
    leadout_minute = 200;
    leadout_second = 0;
    leadout_frame = 0;
    for (index = 0; index < 0x80; index = index + 1) {
        request[index] = 0xA5;
    }
    for (index = 0; index < 64; index = index + 1) {
        DAT_001666c0[index] = 0x11111111;
    }
    for (index = 0; index < 256; index = index + 1) {
        track_control[index] = 0x40;
    }
    for (index = 0; index < 64; index = index + 1) {
        track_calls[index] = -1;
    }
}

static void set_status(int count, int a, int b, int c)
{
    status_count = count;
    status_words[0] = a;
    status_words[1] = b;
    status_words[2] = c;
}

static int table_sentinel(void)
{
    int index;

    for (index = 0; index < 64; index = index + 1) {
        if (DAT_001666c0[index] != 0x11111111) {
            return 0;
        }
    }
    return 1;
}

static int request_sentinel(void)
{
    return request[0] == 0xA5
        && request[0x40] == 0xA5
        && request[0x47] == 0xA5
        && request[0x7f] == 0xA5;
}

static int sequence_ok(int a, int b, int c)
{
    return track_calls[0] == a
        && track_calls[1] == b
        && track_calls[2] == c;
}

int main(void)
{
    int result;

    reset();
    DAT_00166688 = 1;
    result = cd_audio_table_build();
    check('0', result == 1
        && DAT_00166688 == 1
        && calls_260a0 == 0
        && calls_26120 == 0
        && track_count == 0
        && table_sentinel()
        && request_sentinel());

    reset();
    set_status(3, 0x8000, 0x8000, 0x8000);
    first_track = 1;
    last_track_raw = 2;
    result = cd_audio_table_build();
    check('1', result == 1
        && DAT_00166688 == 1
        && calls_260a0 == 3
        && calls_26120 == 3
        && track_count == 0
        && table_sentinel());

    reset();
    set_status(2, 0x8000, 0x0000, 0x0000);
    first_track = 1;
    last_track_raw = 2;
    result = cd_audio_table_build();
    check('2', result == 1
        && DAT_00166688 == 1
        && calls_260a0 == 2
        && calls_26120 == 2
        && track_count == 2
        && sequence_ok(1, 2, -1)
        && DAT_0016668e == 1
        && DAT_00166692 == 2
        && table_sentinel());

    reset();
    set_status(1, 0x0000, 0, 0);
    first_track = 1;
    last_track_raw = 2;
    track_control[1] = 0;
    result = cd_audio_table_build();
    check('3', result == 0
        && DAT_00166688 == 0
        && track_count == 2
        && sequence_ok(1, 2, -1)
        && DAT_0016668e == 1
        && DAT_00166692 == 2
        && DAT_001666c0[0] == 4350
        && DAT_001666c0[1] == 4500
        && DAT_001666c0[2] == 8850
        && DAT_001666c0[3] == 891000
        && DAT_001666c0[4] == 0x11111111
        && request_sentinel());

    reset();
    set_status(1, 0x0000, 0, 0);
    first_track = 1;
    last_track_raw = 0x20;
    result = cd_audio_table_build();
    check('4', result == 1
        && DAT_00166688 == 1
        && track_count == 20
        && track_calls[0] == 1
        && track_calls[19] == 20
        && DAT_0016668e == 1
        && DAT_00166692 == 0x14
        && table_sentinel());

    reset();
    set_status(1, 0x0000, 0, 0);
    first_track = 1;
    last_track_raw = 3;
    track_control[3] = 0;
    result = cd_audio_table_build();
    check('5', result == 0
        && DAT_00166688 == 0
        && track_count == 3
        && sequence_ok(1, 2, 3)
        && DAT_0016668e == 3
        && DAT_00166692 == 3
        && DAT_001666c0[0] == 13350
        && DAT_001666c0[1] == 886500
        && DAT_001666c0[2] == 0x11111111);

    reset();
    set_status(1, 0x0000, 0, 0);
    first_track = 2;
    last_track_raw = 2;
    track_control[2] = 0;
    result = cd_audio_table_build();
    check('6', result == 0
        && DAT_00166688 == 0
        && track_count == 1
        && sequence_ok(2, -1, -1)
        && DAT_0016668e == 2
        && DAT_00166692 == 2
        && DAT_001666c0[0] == 8850
        && DAT_001666c0[1] == 891000
        && DAT_001666c0[2] == 0x11111111);

    reset();
    set_status(1, 0x0000, 0, 0);
    first_track = 19;
    last_track_raw = 0x20;
    track_control[19] = 0;
    result = cd_audio_table_build();
    check('7', result == 0
        && DAT_00166688 == 0
        && track_count == 2
        && sequence_ok(19, 20, -1)
        && DAT_0016668e == 19
        && DAT_00166692 == 0x14
        && DAT_001666c0[0] == 85350
        && DAT_001666c0[1] == 4500
        && DAT_001666c0[2] == 89850
        && DAT_001666c0[3] == 810000
        && DAT_001666c0[4] == 0x11111111);

    put('D');
    quit();
    return 0;
}
