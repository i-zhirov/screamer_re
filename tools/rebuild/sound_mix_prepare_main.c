/* The test harness for the rebuilt `sound_mix_prepare` module.
 *
 * The harness holds the camera record, the saved camera position, the source
 * list, the voice list, the entry list, the object table, the projection
 * table, and the view rectangle.  It runs nine paths:
 *
 *     0  accept: the identity basis keeps the position, the radius is -5, and
 *        the entry records the offset 0 and the radius -5
 *     1  behind: the depth at +0x00 is above zero, so the source is skipped
 *     2  far limit: the depth plus the size stays below the far limit
 *     3  left side: the left projection leaves the rectangle
 *     4  right side: the right projection leaves the rectangle
 *     5  top side: the top projection leaves the rectangle
 *     6  bottom side: the bottom projection leaves the rectangle
 *     7  radius bias: the four object types and the flag 0x40000000 change the
 *        radius as the original does
 *     8  compaction: the middle source is skipped and the two kept voices stay
 *        adjacent
 *     9  transform: the saved position and the Q14 basis rotate the position
 *     D  the exit marker
 *
 * The expected values take their form at run time.  The minimal image does not
 * load a constant array reliably, so the harness compares the scalars directly.
 */
#pragma off (check_stack)

#define FLAG_40 0x40000000

struct camera {
    int field_00;
    int field_04;
    int field_08;
    int field_0c;
    int field_10;
    int field_14;
    int basis_a[9];
    int position[3];
    int basis_b[9];
    int velocity[3];
};

struct mix_emitter {
    int object_index;
    int field_04;
    int position[3];
    int rest[11];
};

struct mix_source {
    int distance;
    struct mix_emitter *emitter;
};

struct mix_object {
    int size;
    int reserved[202];
    int type;
};

struct mix_voice {
    int distance;
    struct mix_emitter *emitter;
    int x;
    int y;
    int z;
};

struct mix_entry {
    unsigned short offset;
    short radius;
};

void sound_mix_prepare(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static struct camera camera;
struct camera *camera_state;
int saved_position[3];
int sound_active_count;
int sound_voice_count;
int *DAT_00021540;
struct mix_source *sound_mix_sources;
struct mix_voice *sound_mix_voices;
struct mix_entry *sound_mix_entries;
int *sound_mix_projection;
int sound_mix_far_limit;
int sound_mix_center_x;
int sound_mix_center_y;
int sound_mix_bound_x;
int sound_mix_bound_y;

static struct mix_source sources[8];
static struct mix_emitter emitters[8];
static struct mix_voice voices[8];
static struct mix_entry entries[8];
static struct mix_object object;
static struct mix_object *object_table[1];
static int projection_storage[512];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(void)
{
    int index;

    for (index = 0; index < 9; index++) {
        camera.basis_b[index] = 0;
    }
    camera.basis_b[0] = 0x4000;
    camera.basis_b[4] = 0x4000;
    camera.basis_b[8] = 0x4000;
    camera_state = &camera;
    saved_position[0] = 0;
    saved_position[1] = 0;
    saved_position[2] = 0;
    object.size = 50;
    object.type = 0;
    object_table[0] = &object;
    DAT_00021540 = (int *)object_table;
    for (index = 0; index < 512; index++) {
        projection_storage[index] = 0x20000;
    }
    sound_mix_projection = &projection_storage[256];
    sound_mix_far_limit = -100000;
    sound_mix_center_x = 0;
    sound_mix_center_y = 0;
    sound_mix_bound_x = 10000;
    sound_mix_bound_y = 10000;
    sound_active_count = 0;
    sound_voice_count = 0;
    for (index = 0; index < 8; index++) {
        sources[index].distance = 0;
        sources[index].emitter = &emitters[index];
        emitters[index].object_index = 0;
        emitters[index].field_04 = 0;
        emitters[index].position[0] = 0;
        emitters[index].position[1] = 0;
        emitters[index].position[2] = 0;
        voices[index].distance = 0;
        voices[index].emitter = 0;
        voices[index].x = 0;
        voices[index].y = 0;
        voices[index].z = 0;
        entries[index].offset = 0;
        entries[index].radius = 0;
    }
    sound_mix_sources = sources;
    sound_mix_voices = voices;
    sound_mix_entries = entries;
}

static void place(int index, int x, int y, int z)
{
    emitters[index].position[0] = x;
    emitters[index].position[1] = y;
    emitters[index].position[2] = z;
}

static void add_source(int index, int distance, int flag)
{
    sources[index].distance = distance;
    sources[index].emitter = (struct mix_emitter *)((unsigned int)&emitters[index]
        | (flag ? FLAG_40 : 0));
}

static int radius_of(int type, int flag)
{
    reset();
    object.type = type;
    place(0, 1000, 2000, -100);
    add_source(0, 400, flag);
    sound_active_count = 1;
    sound_mix_prepare();
    return entries[0].radius;
}

int main(void)
{
    reset();
    object.size = 50;
    place(0, 1000, 2000, -100);
    add_source(0, 400, 0);
    sound_active_count = 1;
    sound_mix_prepare();
    put(sound_voice_count == 1
        && voices[0].distance == 400
        && voices[0].emitter == &emitters[0]
        && voices[0].x == 1000
        && voices[0].y == 2000
        && voices[0].z == -100
        && entries[0].offset == 0
        && entries[0].radius == -5 ? '0' : 'x');

    reset();
    place(0, 1000, 2000, 100);
    add_source(0, 400, 0);
    sound_active_count = 1;
    sound_mix_prepare();
    put(sound_voice_count == 0 ? '1' : 'x');

    reset();
    sound_mix_far_limit = -40;
    place(0, 1000, 2000, -100);
    add_source(0, 400, 0);
    sound_active_count = 1;
    sound_mix_prepare();
    put(sound_voice_count == 0 ? '2' : 'x');

    reset();
    place(0, -1000, 2000, -100);
    add_source(0, 400, 0);
    sound_active_count = 1;
    sound_mix_prepare();
    put(sound_voice_count == 0 ? '3' : 'x');

    reset();
    place(0, 10000, 2000, -100);
    add_source(0, 400, 0);
    sound_active_count = 1;
    sound_mix_prepare();
    put(sound_voice_count == 0 ? '4' : 'x');

    reset();
    place(0, 1000, -1000, -100);
    add_source(0, 400, 0);
    sound_active_count = 1;
    sound_mix_prepare();
    put(sound_voice_count == 0 ? '5' : 'x');

    reset();
    place(0, 1000, 10000, -100);
    add_source(0, 400, 0);
    sound_active_count = 1;
    sound_mix_prepare();
    put(sound_voice_count == 0 ? '6' : 'x');

    put(radius_of(0, 1) == 795
        && radius_of(1, 0) == -1505
        && radius_of(2, 0) == -805
        && radius_of(3, 0) == -6005
        && radius_of(4, 0) == -205 ? '7' : 'x');

    reset();
    place(0, 1000, 2000, -100);
    place(1, 1000, 2000, 100);
    place(2, 3000, 4000, -100);
    add_source(0, 400, 0);
    add_source(1, 400, 0);
    add_source(2, 900, 0);
    sound_active_count = 3;
    sound_mix_prepare();
    put(sound_voice_count == 2
        && voices[0].x == 1000
        && voices[1].x == 3000
        && voices[1].distance == 900
        && entries[0].offset == 0
        && entries[0].radius == -5
        && entries[1].offset == 20
        && entries[1].radius == -7 ? '8' : 'x');

    reset();
    saved_position[0] = 100;
    saved_position[1] = 200;
    saved_position[2] = -300;
    camera.basis_b[0] = 0;
    camera.basis_b[1] = 0x4000;
    camera.basis_b[3] = -0x4000;
    camera.basis_b[4] = 0;
    sound_mix_center_x = 5000;
    sound_mix_center_y = 5000;
    place(0, 1100, 2200, -400);
    add_source(0, 400, 0);
    sound_active_count = 1;
    sound_mix_prepare();
    put(sound_voice_count == 1
        && voices[0].x == -2000
        && voices[0].y == 1000
        && voices[0].z == -100 ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
