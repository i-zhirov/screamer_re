/* The test harness for the rebuilt `music_play` module.
 *
 * The harness holds the request record and the three stubs.  The stubs record
 * their arguments.  The harness runs the four state paths:
 *
 *     0  a negative word at +0x04 sets the state 6 and runs no call
 *     1  a zero ready result sets the state 5 and runs no start
 *     2  the ready track with the select word set starts and selects it
 *     3  the ready track without the select word starts it only
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int music_play(char *record);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char record[0x14];
static int ready_result;
static int got_track;
static char *got_record;
static int got_value;
static int ready_calls;
static int start_calls;
static int select_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int music_ready(int track)
{
    got_track = track;
    ready_calls = ready_calls + 1;
    return ready_result;
}

void music_start(int track)
{
    got_track = track;
    start_calls = start_calls + 1;
}

void music_select(char *music_record, int value)
{
    got_record = music_record;
    got_value = value;
    select_calls = select_calls + 1;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x14; ++index) {
        record[index] = 0;
    }
    ready_calls = 0;
    start_calls = 0;
    select_calls = 0;
    got_track = 0;
    got_record = 0;
    got_value = 0;
    ready_result = 1;
}

int main(void)
{
    int result;

    reset();
    *(int *)record = 3;
    *(int *)(record + 4) = -1;
    result = music_play(record);
    put(result == 6 && *(int *)(record + 0x10) == 6
        && ready_calls == 0 && start_calls == 0 ? '0' : 'x');

    reset();
    *(int *)record = 3;
    ready_result = 0;
    result = music_play(record);
    put(result == 5 && *(int *)(record + 0x10) == 5
        && ready_calls == 1 && got_track == 3 && start_calls == 0 ? '1' : 'x');

    reset();
    *(int *)record = 3;
    *(int *)(record + 0xc) = 1;
    result = music_play(record);
    put(result == 0 && *(int *)(record + 0x10) == 0
        && ready_calls == 1 && start_calls == 1 && got_track == 3
        && select_calls == 1 && got_record == record && got_value == 1 ? '2' : 'x');

    reset();
    *(int *)record = 4;
    result = music_play(record);
    put(result == 0 && *(int *)(record + 0x10) == 0
        && ready_calls == 1 && start_calls == 1 && got_track == 4
        && select_calls == 0 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
