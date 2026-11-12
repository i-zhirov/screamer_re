/* The test harness for the rebuilt `cd_audio_play` module.
 *
 * The harness holds the two guard flags, the track range, the track table, the
 * request buffer, the two saved cells, and the send stub.  The stub counts the
 * driver calls.  The request buffer starts at 0xA5, so every byte outside the
 * written fields must keep that value.
 *
 * The harness runs these paths:
 *
 *     0  both guard bits set, the first track: the request holds the play
 *        command, the device byte, the start time, and the duration
 *     1  guard 1 clear: the return is 1, and no write and no send occur
 *     2  guard 2 clear: the return is 1, and no write and no send occur
 *     3  a track below the first: the return is 1, the cells change, and no
 *        send occurs
 *     4  a track above the last: the return is 1, the cells change, and no
 *        send occurs
 *     5  the last track: the request uses the last table entry
 *     6  the device byte 0x77: the request holds the new subunit
 *     7  a middle track: the request uses the middle table entry
 *     8  two calls: both sends use their own table entry
 *     D  the exit marker
 *
 * The paths 3 and 4 show that the function stores the two cells before the
 * range check.  The paths 0, 5, 7, and 8 show that the table index is the
 * track minus the first track.
 */
#pragma off (check_stack)

int cd_audio_play(int track, int flag);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_0016669e;
unsigned int DAT_00166684;
unsigned int DAT_00166696;
unsigned int DAT_001666ac;
unsigned int DAT_0016668e;
unsigned int DAT_00166692;
unsigned int DAT_001666a4;
unsigned char DAT_00166680;
int DAT_001666c0[0x40];

static char text[8];
static unsigned char request[0x80];
static int send_calls;

void cd_request_send(void)
{
    send_calls = send_calls + 1;
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

    DAT_0016669e = 1;
    DAT_00166684 = 1;
    DAT_00166680 = 0x55;
    DAT_0016668e = 2;
    DAT_00166692 = 5;
    DAT_00166696 = 0;
    DAT_001666ac = 0;
    DAT_001666a4 = (unsigned int)request;
    for (index = 0; index < 0x40; index = index + 2) {
        DAT_001666c0[index] = 0x1000 + index;
        DAT_001666c0[index + 1] = 0x2000 + index;
    }
    for (index = 0; index < 0x80; index = index + 1) {
        request[index] = 0xA5;
    }
    send_calls = 0;
}

static int pristine(void)
{
    int index;

    for (index = 0; index < 0x80; index = index + 1) {
        if (request[index] != 0xA5) {
            return 0;
        }
    }
    return 1;
}

static unsigned int read_dword(int offset)
{
    return (unsigned int)request[offset]
        | ((unsigned int)request[offset + 1] << 8)
        | ((unsigned int)request[offset + 2] << 16)
        | ((unsigned int)request[offset + 3] << 24);
}

static int sentinel_ok(void)
{
    int index;

    for (index = 3; index < 0x0d; index = index + 1) {
        if (request[index] != 0xA5) {
            return 0;
        }
    }
    for (index = 0x16; index < 0x80; index = index + 1) {
        if (request[index] != 0xA5) {
            return 0;
        }
    }
    return 1;
}

static int header_ok(unsigned char device, int entry)
{
    return request[0] == 0x16
        && request[1] == device
        && request[2] == 0x84
        && request[0x0d] == 0
        && read_dword(0x0e) == (unsigned int)DAT_001666c0[entry * 2]
        && read_dword(0x12) == (unsigned int)DAT_001666c0[entry * 2 + 1]
        && sentinel_ok();
}

int main(void)
{
    reset();
    check('0', cd_audio_play(2, 1) == 0 && send_calls == 1
        && header_ok(0x55, 0)
        && DAT_00166696 == 2 && DAT_001666ac == 1);

    reset();
    DAT_0016669e = 0;
    check('1', cd_audio_play(3, 1) == 1 && send_calls == 0
        && pristine()
        && DAT_00166696 == 0 && DAT_001666ac == 0);

    reset();
    DAT_00166684 = 0;
    check('2', cd_audio_play(3, 1) == 1 && send_calls == 0
        && pristine()
        && DAT_00166696 == 0 && DAT_001666ac == 0);

    reset();
    check('3', cd_audio_play(1, 1) == 1 && send_calls == 0
        && pristine()
        && DAT_00166696 == 1 && DAT_001666ac == 1);

    reset();
    check('4', cd_audio_play(6, 1) == 1 && send_calls == 0
        && pristine()
        && DAT_00166696 == 6 && DAT_001666ac == 1);

    reset();
    check('5', cd_audio_play(5, 1) == 0 && send_calls == 1
        && header_ok(0x55, 3));

    reset();
    DAT_00166680 = 0x77;
    check('6', cd_audio_play(2, 1) == 0 && send_calls == 1
        && header_ok(0x77, 0));

    reset();
    check('7', cd_audio_play(3, 1) == 0 && send_calls == 1
        && header_ok(0x55, 1));

    reset();
    check('8', cd_audio_play(2, 1) == 0 && send_calls == 1
        && cd_audio_play(4, 1) == 0 && send_calls == 2
        && header_ok(0x55, 2));

    put('D');
    quit();
    return 0;
}
