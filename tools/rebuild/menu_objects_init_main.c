/* The test harness for the rebuilt `menu_objects_init` module.
 *
 * The harness holds the two name sources, the `ALETTER` name, the object list
 * at `DAT_00006530`, the three slot tables, and the two member tables.  Every
 * callee has a stub.  The `object_load_named` stub records the table, the name,
 * and the slot and writes the call index into the slot on success, so the
 * object field stores go to a known object.  The `atoi` stub parses the text.
 *
 * The harness runs three paths:
 *
 *     0  the normal path: the 27 name loads, the 79 slot loads, the 14 cardat
 *        rows with five numbers each, the 12 trackdat rows with one number
 *        each, the 26 slot binds, and the final init
 *     1  the first name load fails: `bios_video`, `error_report`, and `exit`
 *        run, and the path continues to the end
 *     2  the first car name load fails: the same three calls run at the second
 *        lookup, and the path continues to the end
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).  The harness fills every
 * string from character constants, because the minimal image leaves the
 * initialized data pages zero.
 */
#pragma off (check_stack)

void menu_objects_init(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_00002edc[20];
char DAT_00002ef0[20];
char DAT_0000108f[0x10];
int DAT_001d2d0[160];
int DAT_001d550[0xe * 0x11];
int DAT_001d908[0xc * 0x11];
int DAT_001fd10[96];
int DAT_001fea0[64];
int DAT_00006530[0x78 * 7];

static char text[8];
static char objects[0x78][0x40];
static char cardat_text[14][6][8];
static char trackdat_text[12][4][8];
static char cardat_name[14][8];
static char trackdat_name[12][8];
static char car_name[7][8];
static char map_name[6][8];
static char letter_name[8];

static int create_calls;
static int find_calls;
static void *find_table[64];
static char find_name[64][32];
static int *find_slot[64];
static int find_result[64];
static int next_calls;
static void *next_table[96];
static void *next_slot[96];
static void *next_start[96];
static int bios_calls;
static int bios_mode;
static int error_calls;
static int error_code;
static int exit_calls;
static int exit_code;
static int link_calls;
static char *link_first;
static char *link_second;
static int atoi_calls;
static int bind_calls;
static void *bind_table[32];
static char *bind_name[32];
static void *bind_slot[32];
static int finish_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void copy_text(char *target, const char *source)
{
    while ((*target = *source) != 0) {
        target = target + 1;
        source = source + 1;
    }
}

static int text_is(const char *left, const char *right)
{
    while (*left != 0 && *left == *right) {
        left = left + 1;
        right = right + 1;
    }
    return *left == *right;
}

static void fill(char *target, char a, char b, char c, char d,
                 char e, char f, char g, char h)
{
    target[0] = a;
    target[1] = b;
    target[2] = c;
    target[3] = d;
    target[4] = e;
    target[5] = f;
    target[6] = g;
    target[7] = 0;
}

static void make_number(char *target, int value)
{
    int position;

    position = 0;
    if (value >= 10) {
        target[position] = (char)('0' + value / 10);
        position = position + 1;
    }
    target[position] = (char)('0' + value % 10);
    target[position + 1] = 0;
}

void sub_119cc(void)
{
    create_calls = create_calls + 1;
}

int object_load_named(void *table, char *name, int *slot)
{
    int index;

    index = find_calls;
    if (index < 64) {
        find_table[index] = table;
        copy_text(find_name[index], name);
        find_slot[index] = slot;
        if (find_result[index] == 0) {
            *slot = index;
        }
    }
    find_calls = index + 1;
    if (index < 64) {
        return find_result[index];
    }
    return 0;
}

int object_load_next(void *table, void *slot, void *start)
{
    int index;

    index = next_calls;
    if (index < 96) {
        next_table[index] = table;
        next_slot[index] = slot;
        next_start[index] = start;
    }
    next_calls = index + 1;
    return 0;
}

void bios_video(int mode)
{
    bios_mode = mode;
    bios_calls = bios_calls + 1;
}

void error_report(int code)
{
    error_code = code;
    error_calls = error_calls + 1;
}

void exit(int code)
{
    exit_code = code;
    exit_calls = exit_calls + 1;
}

void object_list_link(char *first, char *second)
{
    link_first = first;
    link_second = second;
    link_calls = link_calls + 1;
}

int atoi(char *source)
{
    int index;
    int negative;
    int value;

    index = 0;
    negative = 0;
    value = 0;
    while (source[index] == ' ' || source[index] == '\t') {
        index = index + 1;
    }
    if (source[index] == '+' || source[index] == '-') {
        negative = source[index] == '-';
        index = index + 1;
    }
    while (source[index] >= '0' && source[index] <= '9') {
        value = value * 10 + (source[index] - '0');
        index = index + 1;
    }
    atoi_calls = atoi_calls + 1;
    return negative ? -value : value;
}

void object_list_bind(void *table, char *name, void *slot)
{
    int index;

    index = bind_calls;
    if (index < 32) {
        bind_table[index] = table;
        bind_name[index] = name;
        bind_slot[index] = slot;
    }
    bind_calls = index + 1;
}

void sub_2dff4(void)
{
    finish_calls = finish_calls + 1;
}

static void reset(void)
{
    int index;
    int group;
    int item;

    for (index = 0; index < 0x78; index++) {
        *(char **)((char *)DAT_00006530 + index * 0x1c + 0xc) = objects[index];
        for (item = 0; item < 0x40; item++) {
            objects[index][item] = 0;
        }
    }
    for (index = 0; index < 20; index++) {
        DAT_00002edc[index] = 0;
        DAT_00002ef0[index] = 0;
    }
    fill(DAT_00002edc, 'A', 'A', 'U', 'T', 'O', '0', '0', 0);
    fill(DAT_00002ef0, 'A', 'M', 'A', 'P', '1', 0, 0, 0);
    fill(DAT_0000108f, 'A', 'L', 'E', 'T', 'T', 'E', 'R', 0);

    for (index = 0; index < 160; index++) {
        DAT_001d2d0[index] = 0;
    }
    for (index = 0; index < 0xe * 0x11; index++) {
        DAT_001d550[index] = 0;
    }
    for (index = 0; index < 0xc * 0x11; index++) {
        DAT_001d908[index] = 0;
    }
    for (index = 0; index < 96; index++) {
        DAT_001fd10[index] = 0;
    }
    for (index = 0; index < 64; index++) {
        DAT_001fea0[index] = 0;
    }

    for (index = 0; index < 7; index++) {
        car_name[index][0] = 'A';
        car_name[index][1] = 'A';
        car_name[index][2] = 'U';
        car_name[index][3] = 'T';
        car_name[index][4] = 'O';
        car_name[index][5] = '0';
        car_name[index][6] = (char)('0' + index);
        car_name[index][7] = 0;
    }
    for (index = 0; index < 6; index++) {
        map_name[index][0] = 'A';
        map_name[index][1] = 'M';
        map_name[index][2] = 'A';
        map_name[index][3] = 'P';
        map_name[index][4] = (char)('1' + index);
        map_name[index][5] = 0;
        map_name[index][6] = 0;
        map_name[index][7] = 0;
    }
    fill(letter_name, 'A', 'L', 'E', 'T', 'T', 'E', 'R', 0);

    for (group = 0; group < 14; group++) {
        cardat_name[group][0] = 'C';
        cardat_name[group][1] = 'D';
        cardat_name[group][2] = (char)('0' + group / 10);
        cardat_name[group][3] = (char)('0' + group % 10);
        cardat_name[group][4] = 0;
        cardat_name[group][5] = 0;
        cardat_name[group][6] = 0;
        cardat_name[group][7] = 0;
        copy_text(cardat_text[group][0], cardat_name[group]);
        make_number(cardat_text[group][1], group + 1);
        make_number(cardat_text[group][2], group + 2);
        make_number(cardat_text[group][3], group + 3);
        make_number(cardat_text[group][4], group + 4);
        make_number(cardat_text[group][5], group + 5);
        for (item = 0; item < 6; item++) {
            DAT_001fd10[group * 6 + item] = (int)cardat_text[group][item];
        }
    }
    for (group = 0; group < 12; group++) {
        trackdat_name[group][0] = 'T';
        trackdat_name[group][1] = 'D';
        trackdat_name[group][2] = (char)('0' + group / 10);
        trackdat_name[group][3] = (char)('0' + group % 10);
        trackdat_name[group][4] = 0;
        trackdat_name[group][5] = 0;
        trackdat_name[group][6] = 0;
        trackdat_name[group][7] = 0;
        copy_text(trackdat_text[group][0], trackdat_name[group]);
        make_number(trackdat_text[group][1], 0);
        make_number(trackdat_text[group][2], 0);
        make_number(trackdat_text[group][3], group + 20);
        for (item = 0; item < 4; item++) {
            DAT_001fea0[group * 4 + item] = (int)trackdat_text[group][item];
        }
    }

    create_calls = 0;
    find_calls = 0;
    for (index = 0; index < 64; index++) {
        find_table[index] = 0;
        find_name[index][0] = 0;
        find_slot[index] = 0;
        find_result[index] = 0;
    }
    next_calls = 0;
    for (index = 0; index < 96; index++) {
        next_table[index] = 0;
        next_slot[index] = 0;
        next_start[index] = 0;
    }
    bios_calls = 0;
    bios_mode = 0;
    error_calls = 0;
    error_code = 0;
    exit_calls = 0;
    exit_code = 0;
    link_calls = 0;
    link_first = 0;
    link_second = 0;
    atoi_calls = 0;
    bind_calls = 0;
    for (index = 0; index < 32; index++) {
        bind_table[index] = 0;
        bind_name[index] = 0;
        bind_slot[index] = 0;
    }
    finish_calls = 0;
}

static int common(void)
{
    int index;

    if (create_calls != 1 || find_calls != 27 || next_calls != 79) {
        return 0;
    }
    if (link_calls != 1 || atoi_calls != 82 || bind_calls != 26) {
        return 0;
    }
    if (finish_calls != 1) {
        return 0;
    }
    if (link_first != (char *)0x6530 || link_second != (char *)0x6350) {
        return 0;
    }
    for (index = 0; index < 27; index++) {
        if (find_table[index] != (void *)0x6530) {
            return 0;
        }
    }
    if (find_slot[0] != (int *)DAT_001d2d0) {
        return 0;
    }
    if (!text_is(find_name[0], letter_name)) {
        return 0;
    }
    for (index = 0; index < 7; index++) {
        int first;
        int second;

        first = 1 + index * 2;
        second = first + 1;
        if (!text_is(find_name[first], car_name[index])) {
            return 0;
        }
        if (!text_is(find_name[second], car_name[index])) {
            return 0;
        }
        if (*(int *)(objects[first] + 0x20) != 0x6000) {
            return 0;
        }
        if (*(int *)(objects[first] + 0x3c) != 0) {
            return 0;
        }
        if (*(int *)(objects[second] + 0x20) != 0xe000) {
            return 0;
        }
        if (*(int *)(objects[second] + 0x3c) != 0) {
            return 0;
        }
    }
    for (index = 0; index < 6; index++) {
        if (!text_is(find_name[15 + index], map_name[index])) {
            return 0;
        }
        if (*(int *)(objects[15 + index] + 0x20) != 0x6000) {
            return 0;
        }
        if (*(int *)(objects[15 + index] + 0x3c) != 0) {
            return 0;
        }
        if (!text_is(find_name[21 + index], map_name[index])) {
            return 0;
        }
        if (*(int *)(objects[21 + index] + 0x20) != 0x6000) {
            return 0;
        }
        if (*(int *)(objects[21 + index] + 0x3c) != 0) {
            return 0;
        }
    }
    if (next_table[0] != (void *)0x6530
        || next_start[0] != (void *)DAT_001d2d0
        || next_slot[0] != (char *)DAT_001d2d0 + 8) {
        return 0;
    }
    if (next_table[78] != (void *)0x6530
        || next_start[78] != (void *)DAT_001d2d0
        || next_slot[78] != (char *)DAT_001d2d0 + 0x4f * 8) {
        return 0;
    }
    for (index = 0; index < 26; index++) {
        if (bind_table[index] != (void *)0x6530) {
            return 0;
        }
        if (bind_name[index] != (char *)0x6350) {
            return 0;
        }
    }
    for (index = 0; index < 14; index++) {
        char *slot;

        slot = (char *)DAT_001d550 + index * 0x44;
        if (bind_slot[index] != (void *)slot) {
            return 0;
        }
        if (!text_is(slot + 0x14, cardat_name[index])) {
            return 0;
        }
        if (*(int *)(slot + 0x30) != index + 1
            || *(int *)(slot + 0x34) != index + 2
            || *(int *)(slot + 0x38) != index + 3
            || *(int *)(slot + 0x3c) != index + 4
            || *(int *)(slot + 0x40) != index + 5) {
            return 0;
        }
    }
    for (index = 0; index < 12; index++) {
        char *slot;

        slot = (char *)DAT_001d908 + index * 0x44;
        if (bind_slot[14 + index] != (void *)slot) {
            return 0;
        }
        if (!text_is(slot + 0x14, trackdat_name[index])) {
            return 0;
        }
        if (*(int *)(slot + 0x30) != index + 20) {
            return 0;
        }
    }
    return 1;
}

static int normal(void)
{
    return common()
        && bios_calls == 0 && error_calls == 0 && exit_calls == 0;
}

static int fail_first(void)
{
    return create_calls == 1
        && find_calls == 27 && next_calls == 79
        && link_calls == 1 && atoi_calls == 82 && bind_calls == 26
        && finish_calls == 1
        && bios_calls == 1 && bios_mode == 3
        && error_calls == 1 && error_code == 5
        && exit_calls == 1 && exit_code == 5;
}

static int fail_middle(void)
{
    return create_calls == 1
        && find_calls == 27 && next_calls == 79
        && link_calls == 1 && atoi_calls == 82 && bind_calls == 26
        && finish_calls == 1
        && bios_calls == 1 && bios_mode == 3
        && error_calls == 1 && error_code == 7
        && exit_calls == 1 && exit_code == 7;
}

int main(void)
{
    reset();
    menu_objects_init();
    put(normal() ? '0' : 'x');

    reset();
    find_result[0] = 5;
    menu_objects_init();
    put(fail_first() ? '1' : 'x');

    reset();
    find_result[1] = 7;
    menu_objects_init();
    put(fail_middle() ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
