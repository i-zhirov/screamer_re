/* The test harness for the rebuilt `cd_audio_q_channel_read` module.
 *
 * The harness holds the device byte, the MSCDEX request buffer, the response
 * status, and the submit stub.  The stub counts the driver calls and writes the
 * response status word at +3 of the request.  The request buffer starts at 0xA5,
 * so every byte outside the written fields must keep that value.
 *
 * The harness runs these paths:
 *
 *     0  status 0x0100: the done bit is clear and the return is 0
 *     1  status 0x8002: the error bit is set and the return is 0x8002
 *     2  status 0x800f: the return is 0x800f
 *     3  status 0x8000: the return is 0x8000, so the low byte is 0
 *     4  status 0x0002: the error bit is clear and the return is 0
 *     5  status 0x8003: the return is 0x8003
 *     6  the device byte 0x77 and the status 0x81ff
 *     7  two calls: the submit count rises and the header stays the same
 *     D  the exit marker
 *
 * The path 4 shows that the return follows the error bit, not the low byte.
 */
#pragma off (check_stack)

int cd_audio_q_channel_read(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_001666a4;
unsigned char DAT_00166680;

static char text[8];
static unsigned char request[0x80];
static int submit_calls;
static unsigned short response_status;

void cd_request_submit(void)
{
    submit_calls = submit_calls + 1;
    *(unsigned short *)((unsigned char *)DAT_001666a4 + 3) = response_status;
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
    response_status = 0x0100;
    for (index = 0; index < 0x80; index = index + 1) {
        request[index] = 0xA5;
    }
    submit_calls = 0;
}

static int sentinel_ok(void)
{
    int index;

    for (index = 5; index <= 0x11; index = index + 1) {
        if (request[index] != 0xA5) {
            return 0;
        }
    }
    for (index = 0x14; index <= 0x3f; index = index + 1) {
        if (request[index] != 0xA5) {
            return 0;
        }
    }
    for (index = 0x4d; index <= 0x7f; index = index + 1) {
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
        && request[2] == 3
        && *(unsigned short *)(request + 0x12) == 0x0b
        && request[0x40] == 0x0c
        && *(unsigned int *)(request + 0x41) == 0
        && *(unsigned int *)(request + 0x45) == 0
        && *(unsigned int *)(request + 0x49) == 0
        && sentinel_ok();
}

int main(void)
{
    int result;

    reset();
    result = cd_audio_q_channel_read();
    check('0', result == 0
        && submit_calls == 1
        && header_ok(0x55));

    reset();
    response_status = 0x8002;
    result = cd_audio_q_channel_read();
    check('1', (result & 0xffff) == 0x8002
        && submit_calls == 1
        && header_ok(0x55));

    reset();
    response_status = 0x800f;
    result = cd_audio_q_channel_read();
    check('2', (result & 0xffff) == 0x800f
        && submit_calls == 1
        && header_ok(0x55));

    reset();
    response_status = 0x8000;
    result = cd_audio_q_channel_read();
    check('3', (result & 0xffff) == 0x8000
        && (result & 0xff) == 0
        && submit_calls == 1
        && header_ok(0x55));

    reset();
    response_status = 0x0002;
    result = cd_audio_q_channel_read();
    check('4', result == 0
        && submit_calls == 1
        && header_ok(0x55));

    reset();
    response_status = 0x8003;
    result = cd_audio_q_channel_read();
    check('5', (result & 0xffff) == 0x8003
        && submit_calls == 1
        && header_ok(0x55));

    reset();
    DAT_00166680 = 0x77;
    response_status = 0x81ff;
    result = cd_audio_q_channel_read();
    check('6', (result & 0xffff) == 0x81ff
        && submit_calls == 1
        && header_ok(0x77));

    result = cd_audio_q_channel_read();
    check('7', (result & 0xffff) == 0x81ff
        && submit_calls == 2
        && header_ok(0x77));

    put('D');
    quit();
    return 0;
}
