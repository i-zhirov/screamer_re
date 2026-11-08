/* The test harness for the rebuilt `menu_enter` module.
 *
 * The harness holds the three state words and the menu frame stub.  The stub
 * counts its calls.  The harness sets the state words to known values, calls
 * the module, and checks the state:
 *
 *     0  `DAT_0011fb64` is -1, the fade is clear, and the frame stub ran once
 *     1  the saved value of `DAT_0011fb58` is restored
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void menu_enter(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_0011fb58;
unsigned int DAT_0011fb64;
unsigned int DAT_0011d224;

static char text[8];
static int frame_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void menu_frame(void)
{
    frame_calls = frame_calls + 1;
}

int main(void)
{
    DAT_0011fb58 = 0x11111111;
    DAT_0011fb64 = 0x22222222;
    DAT_0011d224 = 0x33333333;
    frame_calls = 0;
    menu_enter();
    put(DAT_0011fb64 == 0xffffffff && DAT_0011d224 == 0 && frame_calls == 1
        ? '0' : 'x');
    put(DAT_0011fb58 == 0x11111111 ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
