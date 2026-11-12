/* The test harness for the rebuilt `cd_audio_stop` module.
 *
 * The harness holds the two guard flags, the device byte, the MSCDEX request
 * buffer, and the send stub.  The stub counts the driver calls.  The request
 * buffer starts at 0xA5, so every byte outside the written fields must keep
 * that value.
 *
 * The harness runs these paths:
 *
 *     0  both guard bits set: the header holds 0x0d, 0x85, and the device byte
 *     1  guard 1 clear: no write and no send
 *     2  guard 2 clear: no write and no send
 *     3  guard 1 bit 0 clear with the upper bits set: no write and no send
 *     4  guard 2 bit 0 clear with the upper bits set: no write and no send
 *     5  the device byte 0x77: the header holds the new subunit
 *     6  two calls: the send count rises and the header stays the same
 *     D  the exit marker
 *
 * The paths 3 and 4 show that only bit 0 gates the request.
 */
#pragma off (check_stack)

void cd_audio_stop(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_0016669e;
unsigned int DAT_00166684;
unsigned int DAT_001666a4;
unsigned char DAT_00166680;

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
    DAT_001666a4 = (unsigned int)request;
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

static int sentinel_ok(void)
{
    int index;

    for (index = 3; index < 0x80; index = index + 1) {
        if (request[index] != 0xA5) {
            return 0;
        }
    }
    return 1;
}

static int header_ok(unsigned char device)
{
    return request[0] == 0x0d
        && request[1] == device
        && request[2] == 0x85
        && sentinel_ok();
}

int main(void)
{
    reset();
    cd_audio_stop();
    check('0', send_calls == 1 && header_ok(0x55));

    reset();
    DAT_0016669e = 0;
    cd_audio_stop();
    check('1', send_calls == 0 && pristine());

    reset();
    DAT_00166684 = 0;
    cd_audio_stop();
    check('2', send_calls == 0 && pristine());

    reset();
    DAT_0016669e = 0xfffffffe;
    cd_audio_stop();
    check('3', send_calls == 0 && pristine());

    reset();
    DAT_00166684 = 0xfffffffe;
    cd_audio_stop();
    check('4', send_calls == 0 && pristine());

    reset();
    DAT_00166680 = 0x77;
    cd_audio_stop();
    check('5', send_calls == 1 && header_ok(0x77));

    reset();
    cd_audio_stop();
    cd_audio_stop();
    check('6', send_calls == 2 && header_ok(0x55));

    put('D');
    quit();
    return 0;
}
