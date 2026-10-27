/* The test harness for the rebuilt `free` module.
 *
 * `main` calls the module with a null pointer and with a non-null pointer, and
 * it prints a marker around each call.  The stand-in `nfree` prints `FREE`
 * when the module forwards a pointer.  The output therefore shows the behavior
 * of the module: a null pointer does nothing, and a non-null pointer reaches
 * the run-time `free`.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void free(void *p);
void nfree(void *p);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int marker;
static char text[8];

static void put(char a, char b, char c, char d)
{
    text[0] = a;
    text[1] = b;
    text[2] = c;
    text[3] = d;
    text[4] = 13;
    text[5] = 10;
    text[6] = '$';
    text[7] = 0;
    print(text);
}

void nfree(void *p)
{
    put('F', 'R', 'E', 'E');
}

int main(void)
{
    put('N', 'U', 'L', 'L');
    free(0);
    put('D', 'O', 'N', 'E');
    free(&marker);
    put('A', 'F', 'T', 'R');
    quit();
    return 0;
}
