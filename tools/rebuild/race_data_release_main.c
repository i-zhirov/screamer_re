/* The test harness for the rebuilt `race_data_release` module.
 *
 * The harness holds the archive context `DAT_000062ac`, the music record at
 * `DAT_000062b0`, the list at `DAT_00006350`, the object table at
 * `DAT_00006530`, the CD flag `DAT_000166ac`, and the fourteen callee stubs.
 *
 * The stubs record the call order and the arguments.  The harness runs three
 * paths:
 *
 *     0  the full sequence: the function calls the fourteen targets in the
 *        original order with the original arguments and clears the CD flag
 *     1  a second run with another context value and a sentinel in the object
 *        table: the sequence repeats, the CD flag clears, and the table and
 *        the list stay unchanged
 *     2  a sentinel CD flag and a sentinel music record: the function clears
 *        the flag and leaves the record unchanged
 *     D  the exit marker
 *
 * The original has no branch, so every path checks the same sequence.  The
 * harness checks the `COCKPIT` name behind the free-name call because the
 * original passes the name pointer in `edx`.
 */
#pragma off (check_stack)

void race_data_release(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define MAX_CALLS 32
#define EXPECTED_CALLS 14

enum {
    C_MOUSE = 0,
    C_MUSIC,
    C_FREE_NAME,
    C_RELEASE_ALL,
    C_OBJECT_UNLOAD,
    C_LIBRARY,
    C_SOUND_FX,
    C_TEXT,
    C_SPRITES,
    C_FONTS,
    C_REPLAY,
    C_ATTACK,
    C_GAMESPLINE,
    C_CD
};

int DAT_000062ac;
char DAT_000062b0[0x100];
char DAT_00006350[0x40];
char DAT_00006530[0x40];
int DAT_000166ac;

static char text[8];

static int call_count;
static int call_id[MAX_CALLS];
static int call_arg0[MAX_CALLS];
static int call_arg1[MAX_CALLS];

static void record(int id, int arg0, int arg1)
{
    int index;

    index = call_count;
    if (index < MAX_CALLS) {
        call_id[index] = id;
        call_arg0[index] = arg0;
        call_arg1[index] = arg1;
    }
    call_count = index + 1;
}

void mouse_helper(int value)
{
    record(C_MOUSE, value, 0);
}

void music_bank_stop(void *rec)
{
    record(C_MUSIC, (int)rec, 0);
}

void resource_free_name(int context, char *name)
{
    record(C_FREE_NAME, context, (int)name);
}

void object_list_release_all(void *table, void *list)
{
    record(C_RELEASE_ALL, (int)table, (int)list);
}

void object_unload(void *table)
{
    record(C_OBJECT_UNLOAD, (int)table, 0);
}

void library_free(void)
{
    record(C_LIBRARY, 0, 0);
}

void free_sound_fx(void)
{
    record(C_SOUND_FX, 0, 0);
}

void free_text(void)
{
    record(C_TEXT, 0, 0);
}

void free_sprites(void)
{
    record(C_SPRITES, 0, 0);
}

void free_fonts(void)
{
    record(C_FONTS, 0, 0);
}

void free_replay(void)
{
    record(C_REPLAY, 0, 0);
}

void free_attack(void)
{
    record(C_ATTACK, 0, 0);
}

void free_gamespline(void)
{
    record(C_GAMESPLINE, 0, 0);
}

void cd_audio_stop(void)
{
    record(C_CD, 0, 0);
}

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int name_is(int address, char *wanted)
{
    char *name = (char *)address;
    int index = 0;

    while (wanted[index] != 0) {
        if (name[index] != wanted[index]) {
            return 0;
        }
        index++;
    }
    return name[index] == 0;
}

static int sequence_is_expected(void)
{
    int index;

    if (call_count != EXPECTED_CALLS) {
        return 0;
    }
    for (index = 0; index < EXPECTED_CALLS; index++) {
        if (call_id[index] != index) {
            return 0;
        }
    }
    if (call_arg0[C_MOUSE] != 0) {
        return 0;
    }
    if (call_arg0[C_MUSIC] != (int)DAT_000062b0) {
        return 0;
    }
    if (call_arg0[C_FREE_NAME] != DAT_000062ac) {
        return 0;
    }
    if (!name_is(call_arg1[C_FREE_NAME], "COCKPIT")) {
        return 0;
    }
    if (call_arg0[C_RELEASE_ALL] != (int)DAT_00006530) {
        return 0;
    }
    if (call_arg1[C_RELEASE_ALL] != (int)DAT_00006350) {
        return 0;
    }
    if (call_arg0[C_OBJECT_UNLOAD] != (int)DAT_00006530) {
        return 0;
    }
    return 1;
}

static void reset_calls(void)
{
    call_count = 0;
}

int main(void)
{
    reset_calls();
    DAT_000062ac = 0x11111111;
    DAT_000166ac = -1;
    race_data_release();
    put(sequence_is_expected() && DAT_000166ac == 0 ? '0' : 'x');

    reset_calls();
    DAT_000062ac = 0x22222222;
    DAT_000062b0[0] = 0x5a;
    DAT_00006350[0] = 0x5b;
    DAT_00006530[0] = 0x5c;
    DAT_000166ac = -2;
    race_data_release();
    put(sequence_is_expected() && DAT_000166ac == 0
        && DAT_000062b0[0] == 0x5a
        && DAT_00006350[0] == 0x5b
        && DAT_00006530[0] == 0x5c ? '1' : 'x');

    reset_calls();
    DAT_000062ac = 0x33333333;
    DAT_000062b0[0] = 0x6a;
    DAT_000166ac = 0x7fffffff;
    race_data_release();
    put(sequence_is_expected() && DAT_000166ac == 0
        && DAT_000062b0[0] == 0x6a ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
