/* The test harness for the rebuilt `map_file_load` module.
 *
 * The harness holds the global name buffer `DAT_002154c` and a stub for the
 * two loader calls.  Each stub records the name buffer and the call order, so
 * a missing copy, a swapped call, or a missing call fails the check.
 *
 * The name texts are built at run time in local buffers, because the DOS/4G
 * loader under DOSBox-X leaves the initialized data pages zero.  The expected
 * text uses the same bytes.
 *
 * The paths:
 *
 *     0  the name `MAP1`: the buffer holds `MAP1`, both loaders run once, and
 *        `unr_file_load` runs before `mmp_file_load`
 *     1  the empty name: the buffer holds the empty string and both loaders run
 *     2  the name `MENU`: the buffer holds `MENU` and both loaders run
 *     3  two calls with `TRACK01` and `AB`: the buffer holds `AB`, both loaders
 *        run twice in the same order, and every stub sees `AB`
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void map_file_load(char *name);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_002154c[0x40];

static char text[8];
static char expected[0x40];
static char name_map1[5];
static char name_menu[5];
static char name_track[8];
static char name_ab[3];
static char unr_name[0x40];
static char mmp_name[0x40];
static int unr_calls;
static int mmp_calls;
static int sequence;
static int unr_sequence;
static int mmp_sequence;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int same(char *left, char *right)
{
    int index;

    for (index = 0; left[index] == right[index]; ++index) {
        if (left[index] == 0) {
            return 1;
        }
    }
    return 0;
}

static void copy(char *destination, char *source)
{
    int index;

    for (index = 0; source[index] != 0; ++index) {
        destination[index] = source[index];
    }
    destination[index] = 0;
}

static void set_expected(char a, char b, char c, char d, char e, char f)
{
    expected[0] = a;
    expected[1] = b;
    expected[2] = c;
    expected[3] = d;
    expected[4] = e;
    expected[5] = f;
    expected[6] = 0;
}

static void build_texts(void)
{
    name_map1[0] = 'M';
    name_map1[1] = 'A';
    name_map1[2] = 'P';
    name_map1[3] = '1';
    name_map1[4] = 0;

    name_menu[0] = 'M';
    name_menu[1] = 'E';
    name_menu[2] = 'N';
    name_menu[3] = 'U';
    name_menu[4] = 0;

    name_track[0] = 'T';
    name_track[1] = 'R';
    name_track[2] = 'A';
    name_track[3] = 'C';
    name_track[4] = 'K';
    name_track[5] = '0';
    name_track[6] = '1';
    name_track[7] = 0;

    name_ab[0] = 'A';
    name_ab[1] = 'B';
    name_ab[2] = 0;
}

void unr_file_load(void)
{
    unr_calls = unr_calls + 1;
    unr_sequence = sequence;
    sequence = sequence + 1;
    copy(unr_name, DAT_002154c);
}

void mmp_file_load(void)
{
    mmp_calls = mmp_calls + 1;
    mmp_sequence = sequence;
    sequence = sequence + 1;
    copy(mmp_name, DAT_002154c);
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x40; index++) {
        DAT_002154c[index] = 0;
    }
    expected[0] = 0;
    unr_name[0] = 0;
    mmp_name[0] = 0;
    unr_calls = 0;
    mmp_calls = 0;
    sequence = 0;
    unr_sequence = -1;
    mmp_sequence = -1;
}

static int common_ok(void)
{
    return same(DAT_002154c, expected)
        && same(unr_name, expected)
        && same(mmp_name, expected)
        && unr_sequence < mmp_sequence;
}

int main(void)
{
    build_texts();

    reset();
    set_expected('M', 'A', 'P', '1', 0, 0);
    map_file_load(name_map1);
    put(common_ok() && unr_calls == 1 && mmp_calls == 1 ? '0' : 'x');

    reset();
    expected[0] = 0;
    map_file_load(name_map1 + 4);
    put(common_ok() && unr_calls == 1 && mmp_calls == 1 ? '1' : 'x');

    reset();
    set_expected('M', 'E', 'N', 'U', 0, 0);
    map_file_load(name_menu);
    put(common_ok() && unr_calls == 1 && mmp_calls == 1 ? '2' : 'x');

    reset();
    set_expected('A', 'B', 0, 0, 0, 0);
    map_file_load(name_track);
    map_file_load(name_ab);
    put(common_ok() && unr_calls == 2 && mmp_calls == 2 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
