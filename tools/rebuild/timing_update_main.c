/* The test harness for the rebuilt `timing_update` module.
 *
 * The harness holds the replay state and the replay stubs.  The replay read
 * stub returns a configured value, and the replay write stub counts its calls.
 * The harness runs three timing paths:
 *
 *     0  with the replay flag clear: nothing runs
 *     1  with the replay state clear: the replay write stub runs
 *     2  with the replay state set and the read returning 0: the timing values
 *        advance and the filter entry holds the frame rate value
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void timing_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_00118e1c;
unsigned int DAT_00118e6c;
float DAT_00118e04;
float DAT_00118e08;
float DAT_00118e10;
unsigned int DAT_0010cf08;

static char text[8];
static int read_result;
static int read_calls;
static int write_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int replay_read(void)
{
    read_calls = read_calls + 1;
    return read_result;
}

void replay_write(void)
{
    write_calls = write_calls + 1;
}

int main(void)
{
    DAT_00118e1c = 0;
    DAT_00118e6c = 1;
    read_calls = 0;
    write_calls = 0;
    timing_update();
    put(read_calls == 0 && write_calls == 0 ? '0' : 'x');
    DAT_00118e1c = 0x40;
    DAT_00118e6c = 0;
    read_calls = 0;
    write_calls = 0;
    timing_update();
    put(write_calls == 1 && read_calls == 0 ? '1' : 'x');
    DAT_00118e6c = 1;
    DAT_00118e04 = 2.0f;
    DAT_00118e08 = 4.0f;
    DAT_00118e10 = 0.0f;
    DAT_0010cf08 = 0;
    read_result = 0;
    read_calls = 0;
    timing_update();
    put(read_calls == 1 && DAT_00118e10 == 8.0f
        && DAT_0010cf08 == 0x80e8 + 400 * (int)(1.0f / 2.0f) ? '2' : 'x');
    put('D');
    quit();
    return 0;
}
