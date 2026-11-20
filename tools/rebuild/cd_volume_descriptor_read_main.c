/* The test harness for the rebuilt `cd_volume_descriptor_read` module.
 *
 * The harness holds the driver word `DAT_0016668c`, the subunit
 * `DAT_00166680`, and the request pointer `DAT_001666a4`.  The
 * `cd_request_submit` stub counts the calls, records the request pointer, and
 * writes the response marker 0x5a at the first response byte.
 *
 * The paths:
 *
 *     0  the zero driver word: the result is 0 and the stub is not called
 *     1  the first request buffer: the header fields and the result buffer
 *     2  the second request buffer: the function reads the live pointer
 *     3  the sentinel bytes outside the written fields stay 0xa5
 *     4  two calls in order: the result is the last buffer plus 0x40
 *     D  the exit marker
 *
 * Every path checks the result pointer.  The paths check the call count, the
 * header fields, and the sentinel bytes.
 */
#pragma off (check_stack)

void *cd_volume_descriptor_read(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned short DAT_0016668c;
unsigned int DAT_001666a4;
unsigned char DAT_00166680;

static unsigned char request_a[0x900];
static unsigned char request_b[0x900];
static int submit_calls;
static unsigned int submit_pointer;

void cd_request_submit(void)
{
    submit_calls = submit_calls + 1;
    submit_pointer = DAT_001666a4;
    ((unsigned char *)DAT_001666a4)[0x40] = 0x5a;
}

static char text[8];

static void marker(char value)
{
    text[0] = value;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void check(char value, int condition)
{
    marker(condition ? value : 'x');
}

static void reset(unsigned char *pointer)
{
    int index;

    for (index = 0; index < 0x900; index = index + 1) {
        pointer[index] = 0xA5;
    }
    submit_calls = 0;
    submit_pointer = 0;
}

static int header_ok(unsigned char *pointer, unsigned char subunit)
{
    return pointer[0] == 0x1a
        && pointer[1] == subunit
        && pointer[2] == 0x80
        && pointer[0x0d] == 0
        && *(unsigned short *)(pointer + 0x12) == 1
        && *(unsigned int *)(pointer + 0x14) == 0x10
        && *(unsigned int *)(pointer + 0x18) == 0;
}

static int sentinels_ok(unsigned char *pointer)
{
    int index;

    for (index = 3; index <= 0x0c; index = index + 1) {
        if (pointer[index] != 0xA5) {
            return 0;
        }
    }
    for (index = 0x0e; index <= 0x11; index = index + 1) {
        if (pointer[index] != 0xA5) {
            return 0;
        }
    }
    return pointer[0x1c] == 0xA5;
}

int main(void)
{
    void *result;

    reset(request_a);
    DAT_0016668c = 0;
    DAT_00166680 = 4;
    DAT_001666a4 = (unsigned int)request_a;
    result = cd_volume_descriptor_read();
    check('0', result == 0 && submit_calls == 0 && request_a[0] == 0xA5);

    reset(request_a);
    DAT_0016668c = 3;
    DAT_00166680 = 4;
    DAT_001666a4 = (unsigned int)request_a;
    result = cd_volume_descriptor_read();
    check('1', result == (void *)(request_a + 0x40)
        && submit_calls == 1
        && submit_pointer == (unsigned int)request_a
        && header_ok(request_a, 4));

    reset(request_b);
    DAT_0016668c = 5;
    DAT_00166680 = 7;
    DAT_001666a4 = (unsigned int)request_b;
    result = cd_volume_descriptor_read();
    check('2', result == (void *)(request_b + 0x40)
        && submit_calls == 1
        && submit_pointer == (unsigned int)request_b
        && header_ok(request_b, 7));

    reset(request_a);
    DAT_0016668c = 3;
    DAT_00166680 = 4;
    DAT_001666a4 = (unsigned int)request_a;
    result = cd_volume_descriptor_read();
    check('3', result == (void *)(request_a + 0x40)
        && header_ok(request_a, 4)
        && sentinels_ok(request_a));

    reset(request_a);
    reset(request_b);
    DAT_0016668c = 6;
    DAT_00166680 = 2;
    DAT_001666a4 = (unsigned int)request_a;
    result = cd_volume_descriptor_read();
    DAT_001666a4 = (unsigned int)request_b;
    result = cd_volume_descriptor_read();
    check('4', result == (void *)(request_b + 0x40)
        && submit_calls == 2
        && submit_pointer == (unsigned int)request_b
        && header_ok(request_b, 2));

    marker('D');
    quit();
    return 0;
}
