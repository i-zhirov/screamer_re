/* The test harness for the rebuilt `sprintf` module.
 *
 * The harness holds the `__iob` table and the format engine stub.  The stub
 * fills the buffer with the pattern `TEST`, returns the count 4, and records
 * the fourth argument.  The module writes the terminator, so the program
 * checks the buffer and the returned count.
 *
 *     V  the buffer holds the pattern and the terminator, and the count is 4
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int sprintf(char *buffer, const char *format, ...);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char __iob[256];

static char text[8];
static char buffer[32];
static void *file_arg;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int __vsprintf(char *buf, const char *format, void *args, void *file)
{
    file_arg = file;
    buf[0] = 'T';
    buf[1] = 'E';
    buf[2] = 'S';
    buf[3] = 'T';
    return 4;
}

int main(void)
{
    int count;
    int ok;

    count = sprintf(buffer, "%s");
    ok = count == 4
        && buffer[0] == 'T' && buffer[1] == 'E' && buffer[2] == 'S'
        && buffer[3] == 'T' && buffer[4] == 0
        && file_arg == &__iob[0];
    put(ok ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
