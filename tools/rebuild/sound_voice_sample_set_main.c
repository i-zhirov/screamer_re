/* The test harness for the rebuilt `sound_voice_sample_set` module.
 *
 * The harness holds the setup word `DAT_00008058` and the driver handle
 * `DAT_00018d94`.  The `voice_sample_write` stub records the driver, the voice
 * index, and the record address and counts its calls.
 *
 * The paths:
 *
 *     0  the zero setup word calls nothing
 *     1  the voice 0 and the sample 0 give the table base
 *     2  the voice 1 and the sample 1 add one record of 116 bytes
 *     3  the voice 7 and the sample 3 add three records
 *     4  the voice -1 and the sample 2 pass the negative voice through
 *     5  the sample -1 subtracts one record from the base
 *     6  the driver handle passes through
 *     7  the sample 10 adds ten records
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void sound_voice_sample_set(int voice, int sample);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_00008058;
int DAT_00018d94;

static char text[8];
static int write_calls;
static unsigned int write_driver;
static int write_voice;
static unsigned int write_record;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int voice_sample_write(unsigned int driver, int voice, unsigned int record)
{
    write_calls = write_calls + 1;
    write_driver = driver;
    write_voice = voice;
    write_record = record;
    return 0;
}

static void reset(void)
{
    DAT_00008058 = 1;
    DAT_00018d94 = 0x1234;
    write_calls = 0;
    write_driver = 0;
    write_voice = 0;
    write_record = 0;
}

int main(void)
{
    reset();
    DAT_00008058 = 0;
    sound_voice_sample_set(5, 2);
    put(write_calls == 0 ? '0' : 'x');

    reset();
    sound_voice_sample_set(0, 0);
    put(write_calls == 1 && write_driver == 0x1234 && write_voice == 0
        && write_record == 0x16034 ? '1' : 'x');

    reset();
    sound_voice_sample_set(1, 1);
    put(write_calls == 1 && write_voice == 1
        && write_record == 0x16034 + 116 ? '2' : 'x');

    reset();
    sound_voice_sample_set(7, 3);
    put(write_calls == 1 && write_voice == 7
        && write_record == 0x16034 + 3 * 116 ? '3' : 'x');

    reset();
    sound_voice_sample_set(-1, 2);
    put(write_calls == 1 && write_voice == -1
        && write_record == 0x16034 + 2 * 116 ? '4' : 'x');

    reset();
    sound_voice_sample_set(4, -1);
    put(write_calls == 1 && write_voice == 4
        && write_record == 0x16034 - 116 ? '5' : 'x');

    reset();
    DAT_00018d94 = 0x7f;
    sound_voice_sample_set(2, 5);
    put(write_calls == 1 && write_driver == 0x7f
        && write_record == 0x16034 + 5 * 116 ? '6' : 'x');

    reset();
    sound_voice_sample_set(3, 10);
    put(write_calls == 1 && write_voice == 3
        && write_record == 0x16034 + 10 * 116 ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
