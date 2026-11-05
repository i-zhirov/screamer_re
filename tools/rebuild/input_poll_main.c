/* The test harness for the rebuilt `input_poll` module.
 *
 * The harness holds the five input flags and state words, the device block, the
 * read stub, and the apply stub.  The stubs record their calls.  The harness
 * runs the normal path and four gate cases:
 *
 *     0  the normal path reset the counter, read the input, and applied it
 *     1  a counter below 8 returns 0 and reads nothing
 *     2  a clear first flag returns 0
 *     3  a set device bit 9 skips the apply and returns the state
 *     4  a null device pointer returns 0 and skips the apply
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int input_poll(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_0016669e;
unsigned int DAT_00166684;
unsigned int DAT_00166688;
unsigned int DAT_001666bc;
unsigned int DAT_00166696;
void *DAT_001666a4;
void *DAT_001666ac;

static char text[8];
static unsigned char device[0x50];
static int read_calls;
static unsigned int got_state;
static void *got_device;
static int apply_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void input_read(void)
{
    read_calls = read_calls + 1;
}

void input_apply(unsigned int state, void *device_pointer)
{
    got_state = state;
    got_device = device_pointer;
    apply_calls = apply_calls + 1;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x50; ++index) {
        device[index] = 0;
    }
    DAT_0016669e = 1;
    DAT_00166684 = 1;
    DAT_00166688 = 0;
    DAT_001666bc = 7;
    DAT_00166696 = 0x12345678;
    DAT_001666a4 = device;
    DAT_001666ac = device;
    read_calls = 0;
    apply_calls = 0;
    got_state = 0;
    got_device = 0;
}

int main(void)
{
    int result;

    reset();
    result = input_poll();
    put(DAT_001666bc == 0 && read_calls == 1 && apply_calls == 1
        && got_state == 0x12345678 && got_device == device
        && result == 0x12345678 ? '0' : 'x');

    reset();
    DAT_001666bc = 0;
    result = input_poll();
    put(result == 0 && DAT_001666bc == 1 && read_calls == 0
        && apply_calls == 0 ? '1' : 'x');

    reset();
    DAT_0016669e = 0;
    result = input_poll();
    put(result == 0 && DAT_001666bc == 7 && read_calls == 0 ? '2' : 'x');

    reset();
    *(unsigned short *)(device + 3) = 0x200;
    result = input_poll();
    put(result == 0x12345678 && read_calls == 1 && apply_calls == 0 ? '3' : 'x');

    reset();
    DAT_001666ac = 0;
    result = input_poll();
    put(result == 0 && read_calls == 1 && apply_calls == 0 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
