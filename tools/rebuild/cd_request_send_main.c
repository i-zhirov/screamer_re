/* The test harness for the rebuilt `cd_request_send` module.
 *
 * The harness holds the request header pointer, the CD drive unit, the four
 * RMCS cells, the request buffer, the response status, and the DPMI stub.  The
 * stub checks the RMCS setup and writes the response status word at offset 3 of
 * the request header.
 *
 * The harness runs these paths:
 *
 *     0  status 0x0000: the RMCS far pointer is the request header
 *     1  status 0x8100: the return is the status word
 *     2  status 0x8002: the return is the status word
 *     3  status 0x81ff: the return is the status word
 *     4  a request pointer at +4: the far pointer follows the address
 *     5  the drive unit 5: the RMCS ecx field is 5
 *     6  the drive unit 0x0102: the RMCS ecx field is 0x0102
 *     7  the request pointer 0x000256d7: the low nibble is 7 and the high part
 *        is 0x256d, while the response comes from the request header
 *     D  the exit marker
 *
 * The path 7 shows that the argument sets the RMCS far pointer and the return
 * comes from the request header at DAT_001666a4.
 */
#pragma off (check_stack)

int cd_request_send(unsigned int request);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_001666a4;
unsigned short DAT_0016668c;
unsigned int DAT_001176d0;
unsigned int DAT_001176d8;
unsigned int DAT_001176dc;
unsigned short DAT_001176e2;

static char text[8];
static unsigned char request[0x40];
static unsigned int expected_request;
static unsigned short response_status;
static int stub_calls;
static int stub_ok;

void dpmi_simulate_real_int(unsigned int vector, void *rmcs)
{
    unsigned int low;
    unsigned int high;

    stub_calls = stub_calls + 1;
    low = expected_request & 0xf;
    high = expected_request >> 4;
    stub_ok = vector == 0x2f
        && rmcs == (void *)0x176c0
        && DAT_001176d0 == low
        && DAT_001176e2 == (unsigned short)high
        && DAT_001176dc == 0x1510
        && DAT_001176d8 == DAT_0016668c;
    *(unsigned short *)((unsigned char *)(unsigned int)DAT_001666a4 + 3) =
        response_status;
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

static void reset(unsigned char *address, unsigned short status)
{
    int index;

    DAT_0016668c = 0x0005;
    DAT_001666a4 = (unsigned int)address;
    response_status = status;
    expected_request = (unsigned int)address;
    stub_calls = 0;
    stub_ok = 0;
    DAT_001176d0 = 0xDEADBEEF;
    DAT_001176d8 = 0xDEADBEEF;
    DAT_001176dc = 0xDEADBEEF;
    DAT_001176e2 = 0xDEAD;
    for (index = 0; index < 0x40; index = index + 1) {
        request[index] = 0xA5;
    }
}

int main(void)
{
    unsigned int value;

    reset(request, 0x0000);
    value = (unsigned int)cd_request_send((unsigned int)request);
    check('0', stub_calls == 1 && stub_ok
        && (unsigned short)value == 0x0000);

    reset(request, 0x8100);
    value = (unsigned int)cd_request_send((unsigned int)request);
    check('1', stub_calls == 1 && stub_ok
        && (unsigned short)value == 0x8100);

    reset(request, 0x8002);
    value = (unsigned int)cd_request_send((unsigned int)request);
    check('2', stub_calls == 1 && stub_ok
        && (unsigned short)value == 0x8002);

    reset(request, 0x81ff);
    value = (unsigned int)cd_request_send((unsigned int)request);
    check('3', stub_calls == 1 && stub_ok
        && (unsigned short)value == 0x81ff);

    reset(request + 4, 0x0004);
    value = (unsigned int)cd_request_send((unsigned int)(request + 4));
    check('4', stub_calls == 1 && stub_ok
        && (unsigned short)value == 0x0004);

    reset(request, 0x0005);
    DAT_0016668c = 0x0005;
    value = (unsigned int)cd_request_send((unsigned int)request);
    check('5', stub_calls == 1 && stub_ok
        && (unsigned short)value == 0x0005
        && DAT_001176d8 == 0x0005);

    reset(request, 0x0006);
    DAT_0016668c = 0x0102;
    value = (unsigned int)cd_request_send((unsigned int)request);
    check('6', stub_calls == 1 && stub_ok
        && (unsigned short)value == 0x0006
        && DAT_001176d8 == 0x0102);

    reset(request, 0x0007);
    DAT_0016668c = 0x00ff;
    expected_request = 0x000256d7;
    value = (unsigned int)cd_request_send(0x000256d7);
    check('7', stub_calls == 1 && stub_ok
        && (unsigned short)value == 0x0007
        && DAT_001176d0 == 0x00000007
        && DAT_001176e2 == 0x256d);

    put('D');
    quit();
    return 0;
}
