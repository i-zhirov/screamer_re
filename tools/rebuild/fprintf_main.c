/* The test harness for the rebuilt `fprintf` module.
 *
 * The harness holds the two run-time symbols that the module needs: the
 * standard-output table `__iob` and the print engine `__fprtf`.  The engine
 * stub prints the marker `F` and returns the count 4, so the output shows the
 * chain: the module reaches the engine with the stream, and the count comes
 * back through the module.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int fprintf(void *stream, const char *format, ...);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char __iob[256];

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

int __fprtf(void *fp, const char *format, void *args)
{
    put('F');
    return 4;
}

int main(void)
{
    int count;

    count = fprintf(&__iob[1], "HELLO$");
    put((char)('0' + count));
    quit();
    return 0;
}
