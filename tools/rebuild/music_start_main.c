/* The test harness for the rebuilt `music_start` module.
 *
 * The harness holds the music bank record and the two stubs.  The stubs record
 * their arguments.  The harness runs every state path:
 *
 *     0  the zero status returns with no print and no exit
 *     1  the midi-system message and the exit
 *     2  the sound-driver message and the exit
 *     3  the midi-banks message and the exit
 *     4  the song-initialize message and the exit
 *     5  the song-playing message and the exit
 *     6  the song-not-playing message and the exit
 *     7  an out-of-range status prints nothing and exits
 *     8  the status -1 prints nothing and exits
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void music_start(char *record);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char record[0x14];
static char msg_midi_system[] = "Cannot initialize midi system.";
static char msg_sound_driver[] = "Cannot initialize sound driver.";
static char msg_midi_banks[] = "Invalid midi banks.";
static char msg_song_start[] = "Cannot initialize song.";
static char msg_song_playing[] = "Song still playing";
static char msg_song_idle[] = "Song not playing";

static char *got_format;
static int print_calls;
static int exit_calls;
static int exit_status;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int same(char *first, char *second)
{
    while (*first != 0) {
        if (*first != *second) {
            return 0;
        }
        ++first;
        ++second;
    }
    return *second == 0;
}

int printf(const char *format, ...)
{
    got_format = (char *)format;
    print_calls = print_calls + 1;
    return 0;
}

void exit(int status)
{
    exit_calls = exit_calls + 1;
    exit_status = status;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x14; ++index) {
        record[index] = 0;
    }
    got_format = 0;
    print_calls = 0;
    exit_calls = 0;
    exit_status = 0;
}

static void run_state(int state, int expected_print, char *expected_text,
    int expected_exit, char marker)
{
    reset();
    *(int *)(record + 0x10) = state;
    music_start(record);
    put(print_calls == expected_print && exit_calls == expected_exit
        && (expected_print == 0 || same(got_format, expected_text))
        && (expected_exit == 0 || exit_status == -1) ? marker : 'x');
}

int main(void)
{
    run_state(0, 0, 0, 0, '0');
    run_state(1, 1, msg_midi_system, 1, '1');
    run_state(2, 1, msg_sound_driver, 1, '2');
    run_state(3, 1, msg_midi_banks, 1, '3');
    run_state(4, 1, msg_song_start, 1, '4');
    run_state(5, 1, msg_song_playing, 1, '5');
    run_state(6, 1, msg_song_idle, 1, '6');
    run_state(7, 0, 0, 1, '7');
    run_state(-1, 0, 0, 1, '8');

    put('D');
    quit();
    return 0;
}
