/* The test harness for the rebuilt `sound_source_collect` module.
 *
 * The harness holds the camera record, the row limit, the two source lists of
 * each region, the region array with its -1 end marker, and the output array.
 *
 * The harness runs ten paths:
 *
 *     0  one near source; the row holds a null first list with a nonzero
 *        count, so the function skips it, then one source at (4, 2, 0); the
 *        function appends the metric 5 and the source pointer
 *     1  two sources in the second list: (0, 0, 0) and (-4, 2, 0); the
 *        function appends the metrics 0 and 5 in list order
 *     2  the first list holds a far source at (60000, 0, 0) and a near source
 *        at (2, 2, 0); the second list holds (4, 0, 0); the function skips the
 *        far source and appends 2 then 4
 *     3  one far source; the function appends nothing and still clears the
 *        field at +0x04
 *     4  one source at (100000, 0, 0); the shifted sum truncates to the signed
 *        metric -1794967296, which stays below the limit and is appended
 *     5  the camera at (100, 200, 300) and the source at (104, 202, 300); the
 *        function appends the metric 5
 *     6  two regions and the row limit 1; the function appends the source of
 *        each region
 *     7  two regions and the row limit 0; the function reads only the first
 *        region
 *     8  a source list with the count 0; the function appends nothing and
 *        keeps the field at +0x04
 *     9  the -1 end marker as the first row item; the function appends nothing
 *     D  the exit marker
 *
 * The harness uses only BSS state, because the minimal image does not load
 * the initialized data page.
 */
#pragma off (check_stack)

struct source {
    int field_00;
    int field_04;
    int x;
    int y;
    int z;
    int rest[11];
};

struct region {
    struct source *list_0;
    struct source *list_1;
    int field_08;
    int field_0c;
    int field_10;
    int count_0;
    int count_1;
};

struct camera {
    int field_00;
    int field_04;
    int field_08;
    int x;
    int y;
    int z;
};

int *sound_source_collect(struct region **regions, int *output);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct camera *DAT_0007a7ac;
int DAT_000734f4;

static struct source sources[8];
static struct camera camera;
static struct region region_a;
static struct region region_b;
static struct region *regions[4];
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

static void clear_output(void)
{
    int index;

    for (index = 0; index < 32; index++) {
        output[index] = 0x5a5a5a5a;
    }
}

static void set_source(int index, int x, int y, int z, int field_04)
{
    sources[index].x = x;
    sources[index].y = y;
    sources[index].z = z;
    sources[index].field_04 = field_04;
}

static void clear_region(struct region *region)
{
    region->list_0 = (struct source *)0;
    region->list_1 = (struct source *)0;
    region->field_08 = 0;
    region->field_0c = 0;
    region->field_10 = 0;
    region->count_0 = 0;
    region->count_1 = 0;
}

static void reset(void)
{
    clear_sources();
    clear_output();
    clear_region(&region_a);
    clear_region(&region_b);
    regions[0] = (struct region *)-1;
    regions[1] = (struct region *)-1;
    regions[2] = (struct region *)-1;
    regions[3] = (struct region *)-1;
    camera.field_00 = 0;
    camera.field_04 = 0;
    camera.field_08 = 0;
    camera.x = 0;
    camera.y = 0;
    camera.z = 0;
    DAT_0007a7ac = &camera;
    DAT_000734f4 = 0;
}

int main(void)
{
    int *end;
    int result;

    reset();
    set_source(0, 4, 2, 0, 0x1111);
    region_a.list_0 = &sources[0];
    region_a.count_0 = 1;
    region_a.count_1 = 5;
    regions[0] = &region_a;
    DAT_000734f4 = 1;
    end = sound_source_collect(regions, output);
    result = end == output + 2
        && output[0] == 5 && output[1] == (int)&sources[0]
        && sources[0].field_04 == 0;
    put(result ? '0' : 'x');

    reset();
    set_source(0, 0, 0, 0, 0x2222);
    set_source(1, -4, 2, 0, 0x3333);
    region_a.list_0 = &sources[0];
    region_a.count_0 = 2;
    regions[0] = &region_a;
    DAT_000734f4 = 1;
    end = sound_source_collect(regions, output);
    result = end == output + 4
        && output[0] == 0 && output[1] == (int)&sources[0]
        && output[2] == 5 && output[3] == (int)&sources[1]
        && sources[0].field_04 == 0 && sources[1].field_04 == 0;
    put(result ? '1' : 'x');

    reset();
    set_source(0, 60000, 0, 0, 0x4444);
    set_source(1, 2, 2, 0, 0x5555);
    set_source(2, 4, 0, 0, 0x6666);
    region_a.list_1 = &sources[0];
    region_a.count_1 = 2;
    region_a.list_0 = &sources[2];
    region_a.count_0 = 1;
    regions[0] = &region_a;
    DAT_000734f4 = 1;
    end = sound_source_collect(regions, output);
    result = end == output + 4
        && output[0] == 2 && output[1] == (int)&sources[1]
        && output[2] == 4 && output[3] == (int)&sources[2]
        && sources[0].field_04 == 0 && sources[1].field_04 == 0;
    put(result ? '2' : 'x');

    reset();
    set_source(0, 60000, 0, 0, 0x7777);
    region_a.list_0 = &sources[0];
    region_a.count_0 = 1;
    regions[0] = &region_a;
    DAT_000734f4 = 1;
    end = sound_source_collect(regions, output);
    result = end == output && sources[0].field_04 == 0;
    put(result ? '3' : 'x');

    reset();
    set_source(0, 100000, 0, 0, 0x8888);
    region_a.list_0 = &sources[0];
    region_a.count_0 = 1;
    regions[0] = &region_a;
    DAT_000734f4 = 1;
    end = sound_source_collect(regions, output);
    result = end == output + 2
        && output[0] == -1794967296 && output[1] == (int)&sources[0];
    put(result ? '4' : 'x');

    reset();
    camera.x = 100;
    camera.y = 200;
    camera.z = 300;
    set_source(0, 104, 202, 300, 0x9999);
    region_a.list_0 = &sources[0];
    region_a.count_0 = 1;
    regions[0] = &region_a;
    DAT_000734f4 = 1;
    end = sound_source_collect(regions, output);
    result = end == output + 2
        && output[0] == 5 && output[1] == (int)&sources[0];
    put(result ? '5' : 'x');

    reset();
    set_source(0, 0, 0, 0, 0xaaaa);
    set_source(1, 4, 0, 0, 0xbbbb);
    region_a.list_0 = &sources[0];
    region_a.count_0 = 1;
    region_b.list_0 = &sources[1];
    region_b.count_0 = 1;
    regions[0] = &region_a;
    regions[1] = &region_b;
    regions[2] = (struct region *)-1;
    DAT_000734f4 = 1;
    end = sound_source_collect(regions, output);
    result = end == output + 4
        && output[0] == 0 && output[1] == (int)&sources[0]
        && output[2] == 4 && output[3] == (int)&sources[1];
    put(result ? '6' : 'x');

    reset();
    set_source(0, 0, 0, 0, 0xcccc);
    set_source(1, 4, 0, 0, 0xdddd);
    region_a.list_0 = &sources[0];
    region_a.count_0 = 1;
    region_b.list_0 = &sources[1];
    region_b.count_0 = 1;
    regions[0] = &region_a;
    regions[1] = &region_b;
    regions[2] = (struct region *)-1;
    DAT_000734f4 = 0;
    end = sound_source_collect(regions, output);
    result = end == output + 2
        && output[0] == 0 && output[1] == (int)&sources[0];
    put(result ? '7' : 'x');

    reset();
    set_source(0, 4, 0, 0, 0x1234);
    region_a.list_0 = &sources[0];
    region_a.count_0 = 0;
    regions[0] = &region_a;
    DAT_000734f4 = 1;
    end = sound_source_collect(regions, output);
    result = end == output && sources[0].field_04 == 0x1234;
    put(result ? '8' : 'x');

    reset();
    regions[0] = (struct region *)-1;
    DAT_000734f4 = 3;
    end = sound_source_collect(regions, output);
    result = end == output;
    put(result ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
