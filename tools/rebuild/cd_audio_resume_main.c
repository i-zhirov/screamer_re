/* The test harness for the rebuilt `cd_audio_resume` module.
 *
 * The harness holds the two guard flags, the MSCDEX request buffer, and the
 * send stub.  The stub counts the driver calls.  The request buffer starts at
 * 0xA5, so every byte outside the written fields must keep that value.
 *
 * The harness runs these paths:
 *
 *     0  both guard bits set: the header holds 0x1a, 0x88, and 0x88
 *     1  guard 1 clear: no write and no send
 *     2  guard 2 clear: no write and no send
 *     3  guard 1 bit 0 clear with the upper bits set: no write and no send
 *     4  guard 2 bit 0 clear with the upper bits set: no write and no send
 *     5  both guards all ones: only bit 0 gates the send
 *     6  two calls: the send count rises and the header stays the same
 *     D  the exit marker
 *
 * The paths 3 and 4 show that only bit 0 gates the request.
 */
#pragma off (check_stack)

void cd_audio_resume(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_0016669e;
unsigned int DAT_00166684;
unsigned int DAT_001666a4;

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

static int header_ok(void)
{
    return request[0] == 0x1a
        && request[1] == 0x88
        && request[2] == 0x88
        && sentinel_ok();
}

int main(void)
{
    reset();
    cd_audio_resume();
    check('0', send_calls == 1 && header_ok());

    reset();
    DAT_0016669e = 0;
    cd_audio_resume();
    check('1', send_calls == 0 && pristine());

    reset();
    DAT_00166684 = 0;
    cd_audio_resume();
    check('2', send_calls == 0 && pristine());

    reset();
    DAT_0016669e = 0xfffffffe;
    cd_audio_resume();
    check('3', send_calls == 0 && pristine());

    reset();
    DAT_00166684 = 0xfffffffe;
    cd_audio_resume();
    check('4', send_calls == 0 && pristine());

    reset();
    DAT_0016669e = 0xffffffff;
    DAT_00166684 = 0xffffffff;
    cd_audio_resume();
    check('5', send_calls == 1 && header_ok());

    reset();
    cd_audio_resume();
    cd_audio_resume();
    check('6', send_calls == 2 && header_ok());

    put('D');
    quit();
    return 0;
}
