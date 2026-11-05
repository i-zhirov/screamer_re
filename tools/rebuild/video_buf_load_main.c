/* The test harness for the rebuilt `video_buf_load` module.
 *
 * The harness holds the stream pointer, the two result pointers, and the two
 * stubs.  The lookup stub records the three arguments, returns the configured
 * value, and the release stub counts its calls.  The harness runs the module
 * with a non-null lookup result and then with a null result:
 *
 *     0  with the non-null result: both result pointers hold it, and the
 *        release stub did not run
 *     1  with the null result: the release stub ran with the stream
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void video_buf_load(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_00021574;
void *DAT_00021504;
void *DAT_00121528;

static char text[8];
static void *lookup_result;
static void *got_stream;
static unsigned int got_key;
static void *got_name;
static int release_calls;
static void *release_stream;
static unsigned char stream_obj[8];
static unsigned char buffer[8];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void *resource_get(void *stream, unsigned key, void *name)
{
    got_stream = stream;
    got_key = key;
    got_name = name;
    return lookup_result;
}

void stream_release(void *stream)
{
    release_calls = release_calls + 1;
    release_stream = stream;
}

int main(void)
{
    DAT_00021574 = stream_obj;
    lookup_result = buffer;
    DAT_00021504 = 0;
    DAT_00121528 = 0;
    release_calls = 0;
    video_buf_load();
    put(got_stream == stream_obj && got_key == 0x16c4 && got_name == (void *)0x4b500
        && DAT_00021504 == buffer && DAT_00121528 == buffer && release_calls == 0
        ? '0' : 'x');
    lookup_result = 0;
    DAT_00021504 = buffer;
    DAT_00121528 = buffer;
    release_calls = 0;
    video_buf_load();
    put(release_calls == 1 && release_stream == stream_obj
        && DAT_00021504 == 0 && DAT_00121528 == 0 ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
