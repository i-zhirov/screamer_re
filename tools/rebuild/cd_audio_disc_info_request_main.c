/* The test harness for the rebuilt `cd_audio_disc_info_request` module.
 *
 * The harness holds the device byte, the MSCDEX request buffer, the response
 * pointer, the response dword, and the submit stub.  The stub counts the driver
 * calls, checks that the two payload dwords at +0x41 and +0x45 are clear, and
 * writes the response dword at the response pointer.  The request buffer starts
 * at 0xA5, so every byte outside the written fields must keep that value.
 *
 * The harness runs these paths:
 *
 *     0  response 0: the return is 0 and the header is correct
 *     1  response 0x000000ff: the low byte marks the missing disc
 *     2  the device byte 0x77 and the response 0x12345678
 *     3  two calls: the submit count rises and the header stays the same
 *     4  response 0xffffffff: the return is the raw dword
 *     D  the exit marker
 */
#pragma off (check_stack)

int cd_audio_disc_info_request(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_001666a4;
unsigned int DAT_001666a8;
unsigned char DAT_00166680;

static char text[8];
static unsigned char request[0x80];
static int submit_calls;
static unsigned int response_value;
static int submitted_cleared;

void cd_request_submit(void)
{
    submit_calls = submit_calls + 1;
    submitted_cleared =
        *(unsigned int *)((unsigned char *)DAT_001666a4 + 0x41) == 0
        && *(unsigned int *)((unsigned char *)DAT_001666a4 + 0x45) == 0;
    *(unsigned int *)DAT_001666a8 = response_value;
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
    DAT_001666a8 = (unsigned int)(request + 0x41);
    response_value = 0;
    submitted_cleared = 0;
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
        && request[2] == 3
        && *(unsigned short *)(request + 0x12) == 2
        && request[0x40] == 9
        && submitted_cleared
        && sentinel_ok();
}

int main(void)
{
    int result;

    reset();
    result = cd_audio_disc_info_request();
    check('0', result == 0
        && submit_calls == 1
        && header_ok(0x55));

    reset();
    response_value = 0x000000ff;
    result = cd_audio_disc_info_request();
    check('1', (result & 0xff) == 0xff
        && submit_calls == 1
        && header_ok(0x55));

    reset();
    DAT_00166680 = 0x77;
    response_value = 0x12345678;
    result = cd_audio_disc_info_request();
    check('2', result == 0x12345678
        && submit_calls == 1
        && header_ok(0x77));

    reset();
    result = cd_audio_disc_info_request();
    result = result + cd_audio_disc_info_request();
    check('3', result == 0
        && submit_calls == 2
        && header_ok(0x55));

    reset();
    response_value = 0xffffffff;
    result = cd_audio_disc_info_request();
    check('4', (unsigned int)result == 0xffffffff
        && submit_calls == 1
        && header_ok(0x55));

    put('D');
    quit();
    return 0;
}
