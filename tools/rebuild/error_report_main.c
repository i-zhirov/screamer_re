/* The test harness for the rebuilt `error_report` module.
 *
 * The harness holds the message table and the printer.  The printer is a printf-style
 * function, so the stub counts the calls and prints one marker per call.  The program calls
 * three codes: 0 (one argument), 2 (two arguments), and 20 (the default path).
 *
 * The markers go through the data buffer `text`, because the `print` helper of this harness
 * writes garbage for a string literal (the same fault shows in the output of the driver at
 * work/rebuild/out.txt).
 */
#pragma off (check_stack)

void error_report(int code);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char message_table[0x100];
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

void __cdecl report(char *table, ...)
{
    put('R');
}

int main(void)
{
    error_report(0);
    error_report(2);
    error_report(20);
    put('D');
    quit();
    return 0;
}
