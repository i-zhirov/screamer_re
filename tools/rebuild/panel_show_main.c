/* The test harness for the rebuilt `panel_show` module.
 *
 * The harness holds the five panel state words and the update stub.  The stub
 * counts its calls.  The harness checks the state and the call:
 *
 *     0  the panel state words hold the doubled pair, the visible flag, the
 *        cleared count, and the size
 *     1  the update stub ran once
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void panel_show(int x, int y, int size);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_0011cb14;
unsigned int DAT_0011cb18;
unsigned int DAT_0011cb1c;
unsigned int DAT_0011cb20;
unsigned int DAT_0011cb24;

static char text[8];
static int update_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void panel_update(void)
{
    update_calls = update_calls + 1;
}

int main(void)
{
    DAT_0011cb18 = 0;
    DAT_0011cb1c = 0;
    DAT_0011cb14 = 0;
    DAT_0011cb24 = 0xffffffff;
    DAT_0011cb20 = 0;
    update_calls = 0;
    panel_show(10, 20, 30);
    put(DAT_0011cb18 == 20 && DAT_0011cb1c == 40
        && DAT_0011cb14 == 1 && DAT_0011cb24 == 0
        && DAT_0011cb20 == 30 ? '0' : 'x');
    put(update_calls == 1 ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
