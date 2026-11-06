/* The test harness for the rebuilt `stream_status` module.
 *
 * The harness holds a stream record, sets the status word, calls the module,
 * and checks the returned value.
 *
 *     V  the returned value is the status word
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

unsigned stream_status(void *stream);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char record[0x80];

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
    unsigned int result;

    *(unsigned int *)(record + 0x70) = 0x12345678;
    result = stream_status(record);
    put(result == 0x12345678 ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
