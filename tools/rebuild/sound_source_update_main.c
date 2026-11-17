/* The test harness for the rebuilt `sound_source_update` module.
 *
 * The harness holds the source list, the eight source records, the camera, the
 * world rectangle, the active count, and the output array.
 *
 * The harness runs nine paths:
 *
 *     0  the list head -1; the function returns at once, the active count stays
 *        5, and the output stays unchanged
 *     1  the first list entry holds the bit 0x40000000; the function skips it
 *        and appends the metric 5 and the second source pointer with the bit
 *     2  four sources leave the world rectangle on the four sides; the
 *        function appends only the inside source with the metric 0
 *     3  a far source and a near source; the function skips the far source and
 *        appends the metric 1 of the near source
 *     4  a source at (100000, 0, 0); the shifted sum truncates to the signed
 *        metric -1794967296, which stays below the limit and is appended
 *     5  the camera at (100, 200, 300) and the source at (104, 202, 300); the
 *        function appends the metric 5
 *     6  three sources and the active count 3; the function appends the three
 *        metrics in list order and raises the count to 6
 *     7  a null list entry and the active count 7; the function appends nothing
 *        and keeps the count 7
 *     8  the sources at (51999, 0, 0) and (52000, 0, 0); the first metric stays
 *        below the limit and is appended, and the second metric equals the
 *        limit and is skipped
 *     D  the exit marker
 *
 * The function returns the start of the output, so the harness compares the
 * returned pointer with the output start and checks the active count.
 *
 * The harness uses only BSS state, because the minimal image does not load
 * the initialized data page.
 */
#pragma off (check_stack)

#define LIST_FLAG 0x40000000

struct source {
    int field_00;
    int field_04;
    int x;
    int y;
    int z;
    int rest[11];
};

struct camera {
    int field_00;
    int field_04;
    int field_08;
    int x;
    int y;
    int z;
};

struct rect {
    int min_x;
    int reserved_04;
    int min_y;
    int max_x;
    int reserved_10;
    int max_y;
};

int *sound_source_update(int *output);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct source **DAT_0007451c;
struct camera *DAT_0007a7ac;
struct rect DAT_00072d94;
int DAT_00072488;

static struct source sources[8];
static struct camera camera;
static struct source *list[16];
static int output[32];
static char text[8];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void clear_sources(void)
{
    int index;
    int field;

    for (index = 0; index < 8; index++) {
        sources[index].field_00 = 0;
        sources[index].field_04 = 0;
        sources[index].x = 0;
        sources[index].y = 0;
        sources[index].z = 0;
        for (field = 0; field < 11; field++) {
            sources[index].rest[field] = 0;
        }
    }
}

static void clear_list(void)
{
    int index;

    for (index = 0; index < 16; index++) {
        list[index] = (struct source *)0;
    }
}

static void clear_output(void)
{
    int index;

    for (index = 0; index < 32; index++) {
        output[index] = 0x5a5a5a5a;
    }
}

static void set_source(int index, int x, int y, int z)
{
    sources[index].x = x;
    sources[index].y = y;
    sources[index].z = z;
}

static int marked(int index)
{
    return (int)&sources[index] | LIST_FLAG;
}

static void reset(void)
{
    clear_sources();
    clear_list();
    clear_output();
    camera.field_00 = 0;
    camera.field_04 = 0;
    camera.field_08 = 0;
    camera.x = 0;
    camera.y = 0;
    camera.z = 0;
    DAT_00072d94.min_x = -200000;
    DAT_00072d94.reserved_04 = 0;
    DAT_00072d94.min_y = -200000;
    DAT_00072d94.max_x = 200000;
    DAT_00072d94.reserved_10 = 0;
    DAT_00072d94.max_y = 200000;
    DAT_00072488 = 0;
    DAT_0007451c = list;
    DAT_0007a7ac = &camera;
}

int main(void)
{
    int *end;
    int result;

    reset();
    DAT_0007451c = (struct source **)-1;
    DAT_00072488 = 5;
    end = sound_source_update(output);
    result = end == output && DAT_00072488 == 5
        && output[0] == 0x5a5a5a5a;
    put(result ? '0' : 'x');

    reset();
    set_source(0, 4, 2, 0);
    set_source(1, 4, 2, 0);
    list[0] = (struct source *)((int)&sources[0] | LIST_FLAG);
    list[1] = &sources[1];
    end = sound_source_update(output);
    result = end == output
        && output[0] == 5 && output[1] == marked(1)
        && output[2] == 0x5a5a5a5a && DAT_00072488 == 1;
    put(result ? '1' : 'x');

    reset();
    DAT_00072d94.min_x = -10;
    DAT_00072d94.max_x = 10;
    DAT_00072d94.min_y = -10;
    DAT_00072d94.max_y = 10;
    set_source(0, -11, 0, 0);
    set_source(1, 11, 0, 0);
    set_source(2, 0, 0, -11);
    set_source(3, 0, 0, 11);
    set_source(4, 1, 0, 1);
    list[0] = &sources[0];
    list[1] = &sources[1];
    list[2] = &sources[2];
    list[3] = &sources[3];
    list[4] = &sources[4];
    end = sound_source_update(output);
    result = end == output
        && output[0] == 0 && output[1] == marked(4)
        && output[2] == 0x5a5a5a5a && DAT_00072488 == 1;
    put(result ? '2' : 'x');

    reset();
    set_source(0, 60000, 0, 0);
    set_source(1, 2, 0, 0);
    list[0] = &sources[0];
    list[1] = &sources[1];
    end = sound_source_update(output);
    result = end == output
        && output[0] == 1 && output[1] == marked(1)
        && output[2] == 0x5a5a5a5a && DAT_00072488 == 1;
    put(result ? '3' : 'x');

    reset();
    set_source(0, 100000, 0, 0);
    list[0] = &sources[0];
    end = sound_source_update(output);
    result = end == output
        && output[0] == -1794967296 && output[1] == marked(0)
        && DAT_00072488 == 1;
    put(result ? '4' : 'x');

    reset();
    camera.x = 100;
    camera.y = 200;
    camera.z = 300;
    set_source(0, 104, 202, 300);
    list[0] = &sources[0];
    end = sound_source_update(output);
    result = end == output
        && output[0] == 5 && output[1] == marked(0)
        && DAT_00072488 == 1;
    put(result ? '5' : 'x');

    reset();
    set_source(0, 0, 0, 0);
    set_source(1, 4, 0, 0);
    set_source(2, 8, 0, 0);
    list[0] = &sources[0];
    list[1] = &sources[1];
    list[2] = &sources[2];
    DAT_00072488 = 3;
    end = sound_source_update(output);
    result = end == output
        && output[0] == 0 && output[1] == marked(0)
        && output[2] == 4 && output[3] == marked(1)
        && output[4] == 16 && output[5] == marked(2)
        && output[6] == 0x5a5a5a5a && DAT_00072488 == 6;
    put(result ? '6' : 'x');

    reset();
    DAT_00072488 = 7;
    end = sound_source_update(output);
    result = end == output
        && output[0] == 0x5a5a5a5a && DAT_00072488 == 7;
    put(result ? '7' : 'x');

    reset();
    set_source(0, 51999, 0, 0);
    set_source(1, 52000, 0, 0);
    list[0] = &sources[0];
    list[1] = &sources[1];
    end = sound_source_update(output);
    result = end == output
        && output[0] == 675974000 && output[1] == marked(0)
        && output[2] == 0x5a5a5a5a && DAT_00072488 == 1;
    put(result ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
