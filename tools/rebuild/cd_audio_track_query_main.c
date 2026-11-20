/* The test harness for the rebuilt `cd_audio_track_query` module.
 *
 * The harness holds the device byte, the MSCDEX request buffer, and the submit
 * stub.  The stub counts the driver calls.  The request buffer starts at 0xA5,
 * so every byte outside the written fields must keep that value.
 *
 * The harness runs these paths:
 *
 *     0  track 0 with the device byte 0x55: the header holds the track 0
 *     1  track 1: the header holds the track 1
 *     2  track 0x13 with the device byte 0x77
 *     3  track 0xff: the byte keeps every bit
 *     4  two calls: the submit count rises and the last header holds track 2
 *     5  track 0x80: the high bit stays in the track byte
 *     D  the exit marker
 *
 * Every path checks the header fields, the two zero dwords, and the sentinel
 * bytes.
 */
#pragma off (check_stack)

void cd_audio_track_query(unsigned char track);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_001666a4;
unsigned char DAT_00166680;

static char text[8];
static unsigned char request[0x80];
static int submit_calls;

void cd_request_submit(void)
{
    submit_calls = submit_calls + 1;
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

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static void reset(void)
{
    int index;

    DAT_00166680 = 0x55;
    DAT_001666a4 = (unsigned int)request;
    for (index = 0; index < 0x80; index = index + 1) {
        request[index] = 0xA5;
    }
    submit_calls = 0;
}

static int sentinel_ok(void)
{
    int index;

    for (index = 3; index <= 0x11; index = index + 1) {
        if (request[index] != 0xA5) {
            return 0;
        }
    }
    for (index = 0x14; index <= 0x3f; index = index + 1) {
        if (request[index] != 0xA5) {
            return 0;
        }
    }
    for (index = 0x4a; index <= 0x7f; index = index + 1) {
        if (request[index] != 0xA5) {
            return 0;
        }
    }
    return 1;
}

static int header_ok(unsigned char track, unsigned char device)
{
    return request[0] == 0x1a
        && request[1] == device
        && request[2] == 3
        && *(unsigned short *)(request + 0x12) == 7
        && request[0x40] == 0x0b
        && request[0x41] == track
        && *(unsigned int *)(request + 0x42) == 0
        && *(unsigned int *)(request + 0x46) == 0
        && sentinel_ok();
}

int main(void)
{
    reset();
    cd_audio_track_query(0);
    check('0', submit_calls == 1 && header_ok(0, 0x55));

    reset();
    cd_audio_track_query(1);
    check('1', submit_calls == 1 && header_ok(1, 0x55));

    reset();
    DAT_00166680 = 0x77;
    cd_audio_track_query(0x13);
    check('2', submit_calls == 1 && header_ok(0x13, 0x77));

    reset();
    cd_audio_track_query(0xff);
    check('3', submit_calls == 1 && header_ok(0xff, 0x55));

    reset();
    cd_audio_track_query(0x01);
    cd_audio_track_query(0x02);
    check('4', submit_calls == 2 && header_ok(0x02, 0x55));

    reset();
    cd_audio_track_query(0x80);
    check('5', submit_calls == 1 && header_ok(0x80, 0x55));

    put('D');
    quit();
    return 0;
}
