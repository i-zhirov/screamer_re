/* The test harness for the rebuilt `cd_audio_volume_set` module.
 *
 * The harness holds the two state flags, the two saved volume cells, the
 * device byte, the MSCDEX request buffer, and the submit stub.  The stub
 * counts the driver calls.  The request buffer starts at 0xA5, so every byte
 * outside the written fields must keep that value.
 *
 * The harness runs these paths:
 *
 *     0  the flag at 0x1669e is clear: no call and the cells keep their value
 *     1  the flag at 0x16684 is clear: no call and the cells keep their value
 *     2  the pair (5, 6): the header, the volumes, the zero tail, and one call
 *     3  the same pair again: no second call and the return is 1
 *     4  the pair (5, 7): the right volume changes and the call runs
 *     5  the pair (0, 0): both volumes rise to 1 and the return is 0
 *     6  the pair (0x1234, 0xABCD): the low bytes 0x34 and 0xCD
 *     7  the pair (0x100, 0x200): the low bytes are zero and no clamp runs
 *     8  the device byte 0x77: the request subunit follows the state byte
 *     9  the pair (9, 9): both channels take the same volume
 *     D  the exit marker
 */
#pragma off (check_stack)

int cd_audio_volume_set(int left, int right);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0016669e;
int DAT_00166684;
int DAT_001666b4;
int DAT_001666b0;
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

    DAT_0016669e = 0;
    DAT_00166684 = 0;
    DAT_001666b4 = 0x11111111;
    DAT_001666b0 = 0x22222222;
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
    for (index = 0x49; index <= 0x7f; index = index + 1) {
        if (request[index] != 0xA5) {
            return 0;
        }
    }
    return 1;
}

static int header_ok(unsigned char device)
{
    return request[0] == 0x1a
        && request[1] == device
        && request[2] == 0x0c
        && *(unsigned short *)(request + 0x12) == 9
        && request[0x40] == 3
        && request[0x41] == 0
        && request[0x43] == 1
        && *(unsigned int *)(request + 0x45) == 0
        && sentinel_ok();
}

int main(void)
{
    int result;

    reset();
    DAT_0016669e = 0;
    DAT_00166684 = 1;
    result = cd_audio_volume_set(5, 6);
    check('0', result == 1
        && submit_calls == 0
        && DAT_001666b4 == 0x11111111
        && DAT_001666b0 == 0x22222222
        && sentinel_ok());

    reset();
    DAT_0016669e = 1;
    DAT_00166684 = 0;
    result = cd_audio_volume_set(5, 6);
    check('1', result == 1
        && submit_calls == 0
        && DAT_001666b4 == 0x11111111
        && DAT_001666b0 == 0x22222222
        && sentinel_ok());

    reset();
    DAT_0016669e = 1;
    DAT_00166684 = 1;
    result = cd_audio_volume_set(5, 6);
    check('2', result == 5
        && submit_calls == 1
        && DAT_001666b4 == 5
        && DAT_001666b0 == 6
        && header_ok(0x55)
        && request[0x42] == 5
        && request[0x44] == 6);

    result = cd_audio_volume_set(5, 6);
    check('3', result == 1
        && submit_calls == 1
        && request[0x42] == 5
        && request[0x44] == 6);

    result = cd_audio_volume_set(5, 7);
    check('4', result == 5
        && submit_calls == 2
        && DAT_001666b4 == 5
        && DAT_001666b0 == 7
        && header_ok(0x55)
        && request[0x42] == 5
        && request[0x44] == 7);

    result = cd_audio_volume_set(0, 0);
    check('5', result == 0
        && submit_calls == 3
        && DAT_001666b4 == 0
        && DAT_001666b0 == 0
        && header_ok(0x55)
        && request[0x42] == 1
        && request[0x44] == 1);

    result = cd_audio_volume_set(0x1234, 0xABCD);
    check('6', result == 0x1234
        && submit_calls == 4
        && header_ok(0x55)
        && request[0x42] == 0x34
        && request[0x44] == 0xCD);

    result = cd_audio_volume_set(0x100, 0x200);
    check('7', result == 0x100
        && submit_calls == 5
        && header_ok(0x55)
        && request[0x42] == 0
        && request[0x44] == 0);

    reset();
    DAT_0016669e = 1;
    DAT_00166684 = 1;
    DAT_00166680 = 0x77;
    result = cd_audio_volume_set(1, 2);
    check('8', result == 1
        && submit_calls == 1
        && header_ok(0x77)
        && request[0x42] == 1
        && request[0x44] == 2);

    result = cd_audio_volume_set(9, 9);
    check('9', result == 9
        && submit_calls == 2
        && header_ok(0x77)
        && request[0x42] == 9
        && request[0x44] == 9);

    put('D');
    quit();
    return 0;
}
