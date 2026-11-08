/* The test harness for the rebuilt `input_map` module.
 *
 * The harness holds the input word, the exit byte, the overlay state, the
 * gear state, and the two stubs.  The overlay stub counts its calls, and the
 * reset stub records its argument.  The harness runs eight paths:
 *
 *     0  the normal path runs the overlay update, clears the slot 0x7f9,
 *        advances the gear counter, sets the overlay state, runs the reset
 *        helper, and sets the input bits 0x220
 *     1  the exit request with all four flags clear changes nothing
 *     2  the exit request with one flag set sets bit 1 of the exit byte
 *     3  the input word bit 0x401c returns before every other block
 *     4  the mode 0x10 skips the overlay update and the set overlay state
 *        keeps the gear counter
 *     5  the gear counter at 3 wraps to 0
 *     6  the clear release flag resets the overlay state
 *     7  the set reset gate with control bit 0 clear runs the helper and
 *        leaves the input word unchanged
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

typedef union {
    unsigned short bits;
    unsigned char bytes[2];
} control_flags;

void input_map(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

control_flags DAT_00118e1c;
unsigned char DAT_0000003b;
unsigned char DAT_0000001e;
unsigned char DAT_00000003;
unsigned int DAT_00019880;
unsigned char DAT_001ce44;
unsigned char DAT_0000001b;
unsigned int DAT_001d260;
unsigned int DAT_000007f9;
unsigned char DAT_00000034;
unsigned int DAT_00118f90;
unsigned int DAT_001d2a4;

static char text[8];
static int overlay_calls;
static int reset_calls;
static int reset_value;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void input_overlay_update(void)
{
    overlay_calls = overlay_calls + 1;
}

void input_reset(int value)
{
    reset_calls = reset_calls + 1;
    reset_value = value;
}

static void reset(void)
{
    DAT_00118e1c.bits = 0;
    DAT_0000003b = 0;
    DAT_0000001e = 0;
    DAT_00000003 = 0;
    DAT_00019880 = 0;
    DAT_001ce44 = 0;
    DAT_0000001b = 0;
    DAT_001d260 = 0;
    DAT_000007f9 = 0;
    DAT_00000034 = 0;
    DAT_00118f90 = 0;
    DAT_001d2a4 = 0;
    overlay_calls = 0;
    reset_calls = 0;
    reset_value = 0;
}

int main(void)
{
    reset();
    DAT_00118e1c.bytes[1] = 0x01;
    DAT_0000001b = 1;
    DAT_001d260 = 0;
    DAT_00000034 = 1;
    DAT_00118f90 = 0;
    DAT_001d2a4 = 1;
    DAT_00000003 = 1;
    input_map();
    put(overlay_calls == 1 && DAT_000007f9 == 0
        && DAT_001d2a4 == 2 && DAT_00118f90 == 1
        && reset_calls == 1 && reset_value == 1
        && DAT_00118e1c.bits == 0x0320 && DAT_001ce44 == 0 ? '0' : 'x');

    reset();
    DAT_00118e1c.bytes[1] = 0x20;
    input_map();
    put(DAT_001ce44 == 0 && overlay_calls == 0 && reset_calls == 0
        && DAT_00118f90 == 0 && DAT_001d2a4 == 0 ? '1' : 'x');

    reset();
    DAT_00118e1c.bytes[1] = 0x20;
    DAT_0000003b = 1;
    input_map();
    put(DAT_001ce44 == 2 && overlay_calls == 0 && reset_calls == 0 ? '2' : 'x');

    reset();
    DAT_00118e1c.bits = 0x4000;
    DAT_0000001b = 1;
    DAT_00000034 = 1;
    DAT_00000003 = 1;
    input_map();
    put(overlay_calls == 0 && reset_calls == 0 && DAT_00118f90 == 0
        && DAT_001d2a4 == 0 && DAT_000007f9 == 0 ? '3' : 'x');

    reset();
    DAT_0000001b = 1;
    DAT_001d260 = 0x10;
    DAT_00000034 = 1;
    DAT_00118f90 = 1;
    DAT_001d2a4 = 2;
    DAT_000007f9 = 0x1234;
    input_map();
    put(overlay_calls == 0 && DAT_000007f9 == 0x1234
        && DAT_001d2a4 == 2 && DAT_00118f90 == 1
        && reset_calls == 0 ? '4' : 'x');

    reset();
    DAT_00000034 = 1;
    DAT_00118f90 = 0;
    DAT_001d2a4 = 3;
    input_map();
    put(DAT_001d2a4 == 0 && DAT_00118f90 == 1 ? '5' : 'x');

    reset();
    DAT_00118f90 = 1;
    DAT_00000034 = 0;
    input_map();
    put(DAT_00118f90 == 0 ? '6' : 'x');

    reset();
    DAT_00000003 = 1;
    DAT_00118e1c.bits = 0x0200;
    input_map();
    put(reset_calls == 1 && reset_value == 1
        && DAT_00118e1c.bits == 0x0200 ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
