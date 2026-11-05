/* The test harness for the rebuilt `graphics_load` module.
 *
 * The harness holds the two name buffers that the module fills.  It runs the
 * module with a long pair of names and then with a short pair:
 *
 *     0  the first call stored both names, including the terminators
 *     1  the second call replaced both names, including the empty first name
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void graphics_load(char *name_a, char *name_b);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

extern char graphics_name_a[];
extern char graphics_name_b[];

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

static int same(char *left, char *right)
{
    int index;

    for (index = 0; left[index] == right[index]; ++index) {
        if (left[index] == 0) {
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    graphics_load("DATA\\MENU.PCX", "GFX\\MENU.DAT");
    put(same(graphics_name_a, "DATA\\MENU.PCX")
        && same(graphics_name_b, "GFX\\MENU.DAT") ? '0' : 'x');
    graphics_load("", "X");
    put(same(graphics_name_a, "") && same(graphics_name_b, "X") ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
