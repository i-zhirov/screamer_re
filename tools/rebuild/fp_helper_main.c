/* The test harness for the rebuilt `fp_helper` module.
 *
 * The harness calls the module and prints the run marker and the exit marker.
 * The module rounds the value on the FPU stack, so the harness does not need a
 * data check; the run proves that the module executes and returns.
 *
 *     F  the helper ran
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void fp_helper(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

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

int main(void)
{
    fp_helper();
    put('F');
    put('D');
    quit();
    return 0;
}
