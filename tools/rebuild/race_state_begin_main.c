/* The test harness for the rebuilt `race_state_begin` module.
 *
 * The harness holds the nine state words and sets the old state to 7 before the
 * call.  It checks the five constant bit patterns and the four state stores:
 *
 *     0  the five constants hold the original bit patterns
 *     1  the saved state holds 7, the parameter holds 0x1234, and the state is 1
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void race_state_begin(int value);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float DAT_00120378;
float DAT_00120380;
float DAT_0012037c;
int DAT_001203b0;
float DAT_0012038c;
int DAT_0011fac4;
int DAT_0011fac8;
int DAT_001203b8;
int DAT_0011facc;

static char text[8];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static unsigned int bits(float value)
{
    return *(unsigned int *)&value;
}

int main(void)
{
    DAT_0011fac4 = 7;
    DAT_0011fac8 = 0;
    DAT_001203b8 = 0;
    DAT_0011facc = 0;
    race_state_begin(0x1234);
    put(bits(DAT_00120378) == 0x40000000
        && bits(DAT_00120380) == 0x3f19999a
        && bits(DAT_0012037c) == 0x3f800000
        && DAT_001203b0 == 20000
        && bits(DAT_0012038c) == 0xc1490fdb ? '0' : 'x');
    put(DAT_0011fac8 == -1 && DAT_001203b8 == 7
        && DAT_0011facc == 0x1234 && DAT_0011fac4 == 1 ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
