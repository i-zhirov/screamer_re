/* The test harness for the rebuilt `sound_update` module.
 *
 * The harness holds the driver global, the active count, and the two stubs.
 * The reset stub writes a sentinel to the active count, so the harness proves
 * that the module clears the count after the reset call.  Each stub appends its
 * letter to the call sequence, and the service body stub records the
 * destination and the active count.
 *
 * The harness runs three paths:
 *
 *     0  a set active count: the driver store, the reset call, the count clear,
 *        the destination 0x40330, and the call order
 *     1  a negative active count and the driver 0x6350: the same result
 *     2  two calls in one run: each call resets, clears, and enters the body
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void sound_update(int driver);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int sound_driver_handle;
int sound_active_count;

static char text[8];
static char sequence[8];
static int sequence_length;
static int reset_calls;
static int body_calls;
static int reset_driver_handle;
static int body_driver_handle;
static void *body_destination;
static int body_active_count;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void add(char letter)
{
    if (sequence_length < 7) {
        sequence[sequence_length] = letter;
        sequence_length = sequence_length + 1;
        sequence[sequence_length] = 0;
    }
}

static int order_ok(void)
{
    if (sequence_length == 2) {
        return sequence[0] == 'R' && sequence[1] == 'S';
    }
    if (sequence_length == 4) {
        return sequence[0] == 'R' && sequence[1] == 'S'
            && sequence[2] == 'R' && sequence[3] == 'S';
    }
    return 0;
}

void sound_reset(void)
{
    add('R');
    reset_calls = reset_calls + 1;
    reset_driver_handle = sound_driver_handle;
    sound_active_count = 0x33333333;
}

void sound_service_body(void *destination)
{
    add('S');
    body_calls = body_calls + 1;
    body_driver_handle = sound_driver_handle;
    body_destination = destination;
    body_active_count = sound_active_count;
}

static void reset_test(void)
{
    int index;

    sequence_length = 0;
    for (index = 0; index < 8; index++) {
        sequence[index] = 0;
    }
    reset_calls = 0;
    body_calls = 0;
    reset_driver_handle = 0;
    body_driver_handle = 0;
    body_destination = 0;
    body_active_count = 0x44444444;
    sound_driver_handle = 0x55555555;
    sound_active_count = 0x66666666;
}

static int first_ok(int driver)
{
    return reset_calls == 1 && body_calls == 1
        && sound_driver_handle == driver
        && reset_driver_handle == driver
        && body_driver_handle == driver
        && body_destination == (void *)0x40330
        && body_active_count == 0
        && order_ok();
}

int main(void)
{
    reset_test();
    sound_active_count = 0x22222222;
    sound_update(0x11111111);
    put(first_ok(0x11111111) ? '0' : 'x');

    reset_test();
    sound_active_count = -1;
    sound_update(0x6350);
    put(first_ok(0x6350) ? '1' : 'x');

    reset_test();
    sound_active_count = 0x22222222;
    sound_update(0x7a7a7a7a);
    sound_active_count = 0x22222222;
    sound_update(0x7a7a7a7a);
    put(reset_calls == 2 && body_calls == 2
        && sound_driver_handle == 0x7a7a7a7a
        && reset_driver_handle == 0x7a7a7a7a
        && body_driver_handle == 0x7a7a7a7a
        && body_destination == (void *)0x40330
        && body_active_count == 0
        && order_ok() ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
