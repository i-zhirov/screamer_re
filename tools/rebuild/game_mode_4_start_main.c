/* The test harness for the rebuilt `game_mode_4_start` module.
 *
 * The harness holds the archive pointer, the player index, the replay flag, the
 * map name, the playback block, the ghost index, the state cells, and the eight
 * callee stubs.  Every stub records its call and its arguments.
 *
 * The harness runs these paths:
 *
 *     0  the zero load flag: two player car loads with the pointer 0xf91c, the
 *        scene setup with the zero flag, no resource work, the player index in
 *        the two cells, the zero replay cell, the cleared tail cells, and the
 *        clear flag cells
 *     1  the set load flag with a non-null resource: the ATTACK_RES load, no
 *        fault report, the replay buffer init, the ATTACK_RES release, and the
 *        zero replay cell
 *     2  the set load flag with a null resource: the fault report runs and the
 *        buffer init and the release still run
 *     3  the set replay flag with a failed ghost load: the function returns the
 *        error, no playback starts, the second car load stays unrun, and the
 *        tail cells keep their values
 *     4  the set replay flag with a zero ghost load and the ghost index 0: the
 *        playback takes the index 1, the replay cell takes 1, and the tail runs
 *     5  the two set flag cells: the tail sets both to 1
 *     6  the failed second car load: the function returns the error after the
 *        tail
 *     7  the set replay flag with the ghost index 1: the playback takes the
 *        index 0
 *     8  the failed first car load: the function returns the error and the
 *        scene setup and every later step stay unrun
 *     D  the exit marker
 */
#pragma off (check_stack)

int game_mode_4_start(int load);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_000062ac;
int DAT_0011d2a0;
int DAT_0011d264;
int DAT_0018ec8;
int DAT_0018ea4;
char DAT_0011d294[0x20];
int DAT_0018ed4;
char DAT_0018ed8[0x20];
int DAT_001100a4;
int DAT_0018eb4;
int DAT_0018e90;
int DAT_0000cf40;
int DAT_0018e98;
int DAT_0018ebc;

static char text[8];

static int init_count;
static void *init_car[2];
static int init_load[2];
static int init_result[2];
static int scene_count;
static int scene_value;
static int get_count;
static void *get_archive;
static char *get_name;
static unsigned int get_size;
static void *get_result;
static int report_count;
static void *report_archive;
static int buffer_count;
static int free_count;
static void *free_archive;
static char *free_name;
static int load_count;
static char *load_name;
static int load_result;
static int playback_count;
static char *playback_block;
static int playback_index;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int player_car_init(void *car, int load)
{
    int index;

    index = init_count;
    if (index < 2) {
        init_car[index] = car;
        init_load[index] = load;
    }
    init_count = init_count + 1;
    return index < 2 ? init_result[index] : 0;
}

void scene_setup(int value)
{
    scene_count = scene_count + 1;
    scene_value = value;
}

void *resource_get(void *archive, char *name, unsigned int size)
{
    get_count = get_count + 1;
    get_archive = archive;
    get_name = name;
    get_size = size;
    return get_result;
}

void memory_error_report(void *archive)
{
    report_count = report_count + 1;
    report_archive = archive;
}

void replay_buffer_init(void)
{
    buffer_count = buffer_count + 1;
}

void resource_free_name(void *archive, char *name)
{
    free_count = free_count + 1;
    free_archive = archive;
    free_name = name;
}

int replay_load_atk(char *name)
{
    load_count = load_count + 1;
    load_name = name;
    return load_result;
}

int replay_playback_start(char *block, int index)
{
    playback_count = playback_count + 1;
    playback_block = block;
    playback_index = index;
    return 0;
}

static void reset(void)
{
    DAT_000062ac = (void *)0x1234;
    DAT_0011d2a0 = 0x5a5a5a;
    DAT_0011d264 = 0;
    DAT_0018ec8 = 0x11111111;
    DAT_0018ea4 = 0x22222222;
    DAT_0011d294[0] = 'M';
    DAT_0011d294[1] = 0;
    DAT_0018ed4 = 0;
    DAT_0018ed8[0] = 0;
    DAT_001100a4 = 0x44444444;
    DAT_0018eb4 = 0x55555555;
    DAT_0018e90 = 0x66666666;
    DAT_0000cf40 = 0x77777777;
    DAT_0018e98 = 0;
    DAT_0018ebc = 0;
    init_count = 0;
    init_car[0] = 0;
    init_car[1] = 0;
    init_load[0] = -1;
    init_load[1] = -1;
    init_result[0] = 0;
    init_result[1] = 0;
    scene_count = 0;
    scene_value = -1;
    get_count = 0;
    get_archive = 0;
    get_name = 0;
    get_size = 0;
    get_result = (void *)0x2000;
    report_count = 0;
    report_archive = 0;
    buffer_count = 0;
    free_count = 0;
    free_archive = 0;
    free_name = 0;
    load_count = 0;
    load_name = 0;
    load_result = 0;
    playback_count = 0;
    playback_block = 0;
    playback_index = -1;
}

int main(void)
{
    int result;
    int ok;

    reset();
    DAT_0018e98 = 0;
    DAT_0018ebc = 0;
    result = game_mode_4_start(0);
    ok = result == 0
        && init_count == 2
        && init_car[0] == (void *)0xf91c
        && init_load[0] == 0
        && init_car[1] == (void *)0xf91c
        && init_load[1] == 0
        && scene_count == 1
        && scene_value == 0
        && get_count == 0
        && report_count == 0
        && buffer_count == 0
        && free_count == 0
        && DAT_0018ec8 == 0x5a5a5a
        && DAT_0018ea4 == 0x5a5a5a
        && DAT_001100a4 == 0
        && DAT_0018eb4 == 0
        && DAT_0018e90 == 0
        && DAT_0000cf40 == 1
        && DAT_0018e98 == 0
        && DAT_0018ebc == 0;
    put(ok ? '0' : 'x');

    reset();
    result = game_mode_4_start(1);
    ok = result == 0
        && init_count == 2
        && init_load[0] == 1
        && init_load[1] == 0
        && scene_count == 1
        && scene_value == 1
        && get_count == 1
        && get_archive == DAT_000062ac
        && get_name == (char *)0x465
        && get_size == 0x1f400
        && report_count == 0
        && buffer_count == 1
        && free_count == 1
        && free_archive == DAT_000062ac
        && free_name == (char *)0x470
        && DAT_0018ec8 == 0x5a5a5a
        && DAT_0018ea4 == 0x5a5a5a
        && DAT_001100a4 == 0
        && load_count == 0
        && playback_count == 0;
    put(ok ? '1' : 'x');

    reset();
    get_result = 0;
    result = game_mode_4_start(1);
    ok = result == 0
        && get_count == 1
        && report_count == 1
        && report_archive == DAT_000062ac
        && buffer_count == 1
        && free_count == 1
        && init_count == 2
        && scene_count == 1
        && DAT_001100a4 == 0;
    put(ok ? '2' : 'x');

    reset();
    DAT_0011d264 = 1;
    DAT_0018ed4 = 0;
    DAT_0018e98 = 0x88888888;
    DAT_0018ebc = 0x99999999;
    load_result = 7;
    result = game_mode_4_start(1);
    ok = result == 7
        && init_count == 1
        && scene_count == 1
        && get_count == 1
        && load_count == 1
        && load_name == DAT_0011d294
        && playback_count == 0
        && DAT_0018ec8 == 0x5a5a5a
        && DAT_0018ea4 == 0x5a5a5a
        && DAT_001100a4 == 0x44444444
        && DAT_0018eb4 == 0x55555555
        && DAT_0018e90 == 0x66666666
        && DAT_0000cf40 == 0x77777777
        && DAT_0018e98 == 0x88888888
        && DAT_0018ebc == 0x99999999;
    put(ok ? '3' : 'x');

    reset();
    DAT_0011d264 = 1;
    DAT_0018ed4 = 0;
    load_result = 0;
    result = game_mode_4_start(1);
    ok = result == 0
        && init_count == 2
        && init_load[0] == 1
        && init_load[1] == 0
        && load_count == 1
        && playback_count == 1
        && playback_block == DAT_0018ed8
        && playback_index == 1
        && DAT_001100a4 == 1
        && DAT_0018ec8 == 0x5a5a5a
        && DAT_0018ea4 == 0x5a5a5a
        && DAT_0018eb4 == 0
        && DAT_0018e90 == 0
        && DAT_0000cf40 == 1
        && get_count == 1
        && buffer_count == 1
        && free_count == 1;
    put(ok ? '4' : 'x');

    reset();
    DAT_0018e98 = 5;
    DAT_0018ebc = 5;
    result = game_mode_4_start(0);
    ok = result == 0
        && DAT_0018e98 == 1
        && DAT_0018ebc == 1
        && init_count == 2
        && DAT_001100a4 == 0
        && DAT_0018eb4 == 0
        && DAT_0018e90 == 0
        && DAT_0000cf40 == 1;
    put(ok ? '5' : 'x');

    reset();
    init_result[0] = 0;
    init_result[1] = 3;
    result = game_mode_4_start(0);
    ok = result == 3
        && init_count == 2
        && init_load[0] == 0
        && init_load[1] == 0
        && DAT_0018eb4 == 0
        && DAT_0018e90 == 0
        && DAT_0000cf40 == 1;
    put(ok ? '6' : 'x');

    reset();
    DAT_0011d264 = 1;
    DAT_0018ed4 = 1;
    load_result = 0;
    result = game_mode_4_start(0);
    ok = result == 0
        && playback_count == 1
        && playback_block == DAT_0018ed8
        && playback_index == 0
        && DAT_001100a4 == 1;
    put(ok ? '7' : 'x');

    reset();
    init_result[0] = 9;
    result = game_mode_4_start(0x22);
    ok = result == 9
        && init_count == 1
        && scene_count == 0
        && get_count == 0
        && buffer_count == 0
        && free_count == 0
        && load_count == 0
        && playback_count == 0;
    put(ok ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
