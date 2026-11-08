/* The test harness for the rebuilt `stream_seek` module.
 *
 * The harness holds a stream record and the `fseek` stub.  The stub records
 * its arguments and its call count.  The harness runs eight paths:
 *
 *     0  the origin 0 seek: the position and the buffer index from the start
 *     1  the origin 1 seek: the position and the buffer index add the offset
 *     2  the origin 2 seek: the position from the size and the index from
 *        the field at +0x6c
 *     3  the origin 2 seek past the end: the status 4 and the -1 return
 *     4  the origin 0 seek past the end: the status 4 and the -1 return
 *     5  an invalid origin keeps the record and still calls `fseek`
 *     6  the origin 1 seek past the end: the status 4 and the -1 return
 *     7  the origin 0 seek with a negative offset before the start
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int stream_seek(char *stream, int offset, int origin);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char record[0xc0];
static int file_tag;

static int seek_calls;
static void *seek_file;
static int seek_offset;
static int seek_origin;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int fseek(void *file, int offset, int origin)
{
    seek_file = file;
    seek_offset = offset;
    seek_origin = origin;
    seek_calls = seek_calls + 1;
    return 0;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0xc0; ++index) {
        record[index] = 0;
    }
    *(void **)record = &file_tag;
    *(int *)(record + 4) = 100;
    *(int *)(record + 8) = 200;
    *(int *)(record + 0x6c) = 40;
    seek_calls = 0;
}

int main(void)
{
    int result;

    reset();
    *(int *)(record + 0xc) = 0;
    *(int *)(record + 0x18) = 0;
    *(int *)(record + 0xac) = 0x55;
    result = stream_seek((char *)record, 25, 0);
    put(result == 0 && *(int *)(record + 0xc) == 125
        && *(int *)(record + 0x18) == 25 && *(int *)(record + 0xac) == 0x55
        && seek_calls == 1 && seek_file == &file_tag && seek_offset == 125
        && seek_origin == 0 ? '0' : 'x');

    reset();
    *(int *)(record + 0xc) = 50;
    *(int *)(record + 0x18) = 10;
    result = stream_seek((char *)record, 30, 1);
    put(result == 0 && *(int *)(record + 0xc) == 80
        && *(int *)(record + 0x18) == 40 && seek_calls == 1
        && seek_offset == 80 && seek_origin == 0 ? '1' : 'x');

    reset();
    *(int *)(record + 0xc) = 0;
    *(int *)(record + 0x18) = 0;
    result = stream_seek((char *)record, -50, 2);
    put(result == 0 && *(int *)(record + 0xc) == 150
        && *(int *)(record + 0x18) == -10 && seek_calls == 1
        && seek_offset == 150 && seek_origin == 0 ? '2' : 'x');

    reset();
    *(int *)(record + 0xc) = 0;
    *(int *)(record + 0x18) = 0;
    result = stream_seek((char *)record, 60, 2);
    put(result == -1 && *(int *)(record + 0xc) == 260
        && *(int *)(record + 0x18) == 100 && *(int *)(record + 0xac) == 4
        && seek_calls == 1 && seek_offset == 260 ? '3' : 'x');

    reset();
    *(int *)(record + 4) = 150;
    *(int *)(record + 0xc) = 0;
    *(int *)(record + 0x18) = 0;
    result = stream_seek((char *)record, 100, 0);
    put(result == -1 && *(int *)(record + 0xc) == 250
        && *(int *)(record + 0x18) == 100 && *(int *)(record + 0xac) == 4
        && seek_calls == 1 && seek_offset == 250 ? '4' : 'x');

    reset();
    *(int *)(record + 0xc) = 70;
    *(int *)(record + 0x18) = 7;
    result = stream_seek((char *)record, 10, 3);
    put(result == 0 && *(int *)(record + 0xc) == 70
        && *(int *)(record + 0x18) == 7 && seek_calls == 1
        && seek_offset == 70 && seek_origin == 0 ? '5' : 'x');

    reset();
    *(int *)(record + 0xc) = 195;
    *(int *)(record + 0x18) = 5;
    result = stream_seek((char *)record, 10, 1);
    put(result == -1 && *(int *)(record + 0xc) == 205
        && *(int *)(record + 0x18) == 15 && *(int *)(record + 0xac) == 4
        && seek_calls == 1 && seek_offset == 205 ? '6' : 'x');

    reset();
    *(int *)(record + 0xc) = 0;
    *(int *)(record + 0x18) = 0;
    result = stream_seek((char *)record, -50, 0);
    put(result == 0 && *(int *)(record + 0xc) == 50
        && *(int *)(record + 0x18) == -50 && seek_calls == 1
        && seek_offset == 50 ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
