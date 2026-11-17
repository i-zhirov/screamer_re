/* The test harness for the rebuilt `screen_mode_set` module.
 *
 * The harness holds two screen mode records, the record pointer, and the
 * selector setup stub.  The stub counts its calls.  Before each call the
 * harness fills the target record with the byte 0x11, so the check proves the
 * function writes only the fields from `+0x14` up.
 *
 * The four paths are:
 *
 *     0  the race mode 0x140 by 0x190 with the value 0x104
 *     1  the menu mode 0x140 by 0x190 with the value 0xc8
 *     2  the negative width, height, and value: the arithmetic shift rounds
 *        down and the signed division truncates toward zero
 *     3  the second record with the unity values: the function follows the
 *        record pointer and leaves the first record untouched
 *     D  the exit marker
 *
 * The expected values come from the original arithmetic in this file.  The
 * harness uses no initialized static data, because the minimal image leaves
 * the initialized data pages zero under DOS/4G.
 */
#pragma off (check_stack)

void screen_mode_set(int width, int height, int value);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define INFO_FIELDS 14
#define INFO_FIRST 0x14
#define INFO_SIZE 0x4c

struct screen_mode_info {
    unsigned char pad_00[0x14];
    int field_14;
    int field_18;
    int field_1c;
    int field_20;
    int field_24;
    int field_28;
    int field_2c;
    int field_30;
    int field_34;
    int field_38;
    int field_3c;
    int field_40;
    int field_44;
    int field_48;
};

struct screen_mode_info *PTR_DAT_0008a844;

static char text[8];
static int commit_count;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void video_selector_setup(void)
{
    commit_count = commit_count + 1;
}

static void info_fill(struct screen_mode_info *info, unsigned char value)
{
    unsigned char *bytes = (unsigned char *)info;
    int index;

    for (index = 0; index < INFO_SIZE; index++) {
        bytes[index] = value;
    }
}

static int prefix_ok(struct screen_mode_info *info)
{
    unsigned char *bytes = (unsigned char *)info;
    int index;

    for (index = 0; index < INFO_FIRST; index++) {
        if (bytes[index] != 0x11) {
            return 0;
        }
    }
    return 1;
}

static int info_ok(struct screen_mode_info *info, const int *expected)
{
    int *fields = (int *)((char *)info + INFO_FIRST);
    int index;

    if (!prefix_ok(info)) {
        return 0;
    }
    for (index = 0; index < INFO_FIELDS; index++) {
        if (fields[index] != expected[index]) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    struct screen_mode_info mode_a;
    struct screen_mode_info mode_b;
    int expected[INFO_FIELDS];

    commit_count = 0;
    PTR_DAT_0008a844 = &mode_a;

    info_fill(&mode_a, 0x11);
    expected[0] = 639;
    expected[1] = 400;
    expected[2] = 319;
    expected[3] = 200;
    expected[4] = 0x027f0000;
    expected[5] = 0x01900000;
    expected[6] = 320;
    expected[7] = 260;
    expected[8] = 160;
    expected[9] = 130;
    expected[10] = 0x01400000;
    expected[11] = 0x01040000;
    expected[12] = 640;
    expected[13] = 401;
    screen_mode_set(0x140, 0x190, 0x104);
    put(info_ok(&mode_a, expected) && commit_count == 1 ? '0' : 'x');

    info_fill(&mode_a, 0x11);
    expected[0] = 639;
    expected[1] = 400;
    expected[2] = 319;
    expected[3] = 200;
    expected[4] = 0x027f0000;
    expected[5] = 0x01900000;
    expected[6] = 320;
    expected[7] = 200;
    expected[8] = 160;
    expected[9] = 100;
    expected[10] = 0x01400000;
    expected[11] = 0x00c80000;
    expected[12] = 640;
    expected[13] = 401;
    screen_mode_set(0x140, 0x190, 0xc8);
    put(info_ok(&mode_a, expected) && commit_count == 2 ? '1' : 'x');

    info_fill(&mode_a, 0x11);
    expected[0] = -7;
    expected[1] = -5;
    expected[2] = -4;
    expected[3] = -3;
    expected[4] = -458752;
    expected[5] = -327680;
    expected[6] = -3;
    expected[7] = -7;
    expected[8] = -2;
    expected[9] = -4;
    expected[10] = -196608;
    expected[11] = -458752;
    expected[12] = -6;
    expected[13] = -4;
    screen_mode_set(-3, -5, -7);
    put(info_ok(&mode_a, expected) && commit_count == 3 ? '2' : 'x');

    info_fill(&mode_a, 0x11);
    info_fill(&mode_b, 0x11);
    PTR_DAT_0008a844 = &mode_b;
    expected[0] = 1;
    expected[1] = 1;
    expected[2] = 0;
    expected[3] = 0;
    expected[4] = 65536;
    expected[5] = 65536;
    expected[6] = 1;
    expected[7] = 1;
    expected[8] = 0;
    expected[9] = 0;
    expected[10] = 65536;
    expected[11] = 65536;
    expected[12] = 2;
    expected[13] = 2;
    screen_mode_set(1, 1, 1);
    put(info_ok(&mode_b, expected) && prefix_ok(&mode_a)
        && commit_count == 4 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
