/* The test harness for the rebuilt `cd_request_submit` module.
 *
 * The harness holds the request buffer and the `cd_request_send` stub.  The
 * stub records the call count, the request pointer, and returns a chosen status
 * word.  The harness computes the expected low nibble and the expected high
 * part from the live buffer address, so every path checks the real-mode
 * transfer address of the block at request + 0x40.
 *
 * The harness runs these paths:
 *
 *     0  the buffer base with status 0x0000: one call with the buffer pointer
 *     1  the buffer at +4 with status 0x8002: the words follow the new address
 *     2  status 0x81ff: the return forwards the sender status
 *     3  the bytes outside +0x0e through +0x11 keep the sentinel 0xa5
 *     4  two calls in a row: the stub counts both calls
 *     D  the exit marker
 *
 * Every path checks the two transfer words, the request pointer, and the
 * forwarded return value.
 */
#pragma off (check_stack)

int cd_request_submit(unsigned int request);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char request[0x60];
static int send_calls;
static unsigned int send_arg;
static int send_status;

int cd_request_send(unsigned int pointer)
{
    send_calls = send_calls + 1;
    send_arg = pointer;
    return send_status;
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

static void reset(unsigned short status)
{
    int index;

    for (index = 0; index < 0x60; index = index + 1) {
        request[index] = 0xA5;
    }
    send_calls = 0;
    send_arg = 0;
    send_status = status;
}

static int words_ok(unsigned char *pointer)
{
    unsigned int address;
    unsigned int high;
    unsigned int low;

    address = (unsigned int)pointer + 0x40;
    high = address >> 4;
    low = address & 0xf;
    return *(unsigned short *)(pointer + 0xe) == (unsigned short)low
        && *(unsigned short *)(pointer + 0x10) == (unsigned short)high;
}

static int sentinels_ok(unsigned char *pointer)
{
    return pointer[0x0c] == 0xA5
        && pointer[0x0d] == 0xA5
        && pointer[0x12] == 0xA5
        && pointer[0x13] == 0xA5;
}

int main(void)
{
    int value;

    reset(0x0000);
    value = cd_request_submit((unsigned int)request);
    check('0', send_calls == 1
        && send_arg == (unsigned int)request
        && words_ok(request)
        && value == 0x0000);

    reset(0x8002);
    value = cd_request_submit((unsigned int)(request + 4));
    check('1', send_calls == 1
        && send_arg == (unsigned int)(request + 4)
        && words_ok(request + 4)
        && value == 0x8002);

    reset(0x81ff);
    value = cd_request_submit((unsigned int)request);
    check('2', send_calls == 1
        && send_arg == (unsigned int)request
        && words_ok(request)
        && value == 0x81ff);

    reset(0x0100);
    value = cd_request_submit((unsigned int)request);
    check('3', words_ok(request)
        && sentinels_ok(request)
        && value == 0x0100);

    reset(0x0004);
    value = cd_request_submit((unsigned int)request);
    value = cd_request_submit((unsigned int)request);
    check('4', send_calls == 2
        && send_arg == (unsigned int)request
        && words_ok(request)
        && value == 0x0004);

    put('D');
    quit();
    return 0;
}
