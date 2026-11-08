/* The test harness for the rebuilt `sound_voice_setup` module.
 *
 * The harness holds the voice context, the voice record area, the voice entry
 * list, the emitter array, the object array, the handle table, the camera
 * position, the scale, and the two basis builder stubs.
 *
 * The first stub records its argument.  The second stub records its two
 * arguments and sets the flag 0x7b518, as the original at 0x8b51c does.
 *
 * The harness runs these paths:
 *
 *     0  a plain emitter: the fields, the camera position, the basis values,
 *        the level, the object, and the scaled field all take their values
 *     1  the near flag 0x40000000: the pointer clears the flag, the radius
 *        becomes pointer + 0x40, and the record keeps the raw pointer
 *     2  the event flag 0x8000 with a negative level: the second builder runs,
 *        the flag 0x7b518 sets, and the negative object field stays unchanged
 *     3  the first of two calls: the entry pointer advances by four bytes
 *     4  the second call: the pointer advances again and the second emitter
 *        takes over
 *     D  the exit marker
 */
#pragma off (check_stack)

void sound_voice_setup(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define NEAR_FLAG 0x40000000

struct voice_emitter {
    int object_index;
    int flags;
    int x;
    int y;
    int z;
    int basis0;
    int basis1;
    int basis2;
    int level;
    int reserved_24[6];
    int field_3c;
};

struct voice_object {
    int reserved_00[4];
    int field_10;
    int field_14;
};

unsigned char sound_voice_context[0xdc];
unsigned char sound_voice_records[0x40];
unsigned char *sound_voice_entry;
void **sound_handle_table;
int sound_emitter_scale;
int sound_camera_x;
int sound_camera_y;
int sound_camera_z;
int sound_voice_volume;
int sound_voice_shift;
int sound_voice_field;
int sound_voice_flag;

static struct voice_emitter emitters[3];
static struct voice_object objects[2];
static void *handles[2];
static unsigned char entry_list[16];
static char text[8];

static int basis_calls;
static void *basis_argument;
static int event_calls;
static void *event_basis;
static void *event_emitter;

void sound_basis_build(void *basis)
{
    basis_calls = basis_calls + 1;
    basis_argument = basis;
}

void sound_basis_build_event(void *basis, void *emitter)
{
    event_calls = event_calls + 1;
    event_basis = basis;
    event_emitter = emitter;
    sound_voice_flag = 1;
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

static int ctx_get(int offset)
{
    return *(int *)(sound_voice_context + offset);
}

static void ctx_clear(void)
{
    int index;

    for (index = 0; index < 0xdc; index++) {
        sound_voice_context[index] = 0;
    }
    for (index = 0; index < 0x40; index++) {
        sound_voice_records[index] = 0;
    }
}

static void set_entry(int index, int offset)
{
    *(unsigned short *)(entry_list + index * 4) = (unsigned short)offset;
}

static void set_record_emitter(int offset, int emitter)
{
    *(int *)(sound_voice_records + offset + 4) = emitter;
}

static void reset(void)
{
    ctx_clear();
    sound_voice_entry = entry_list;
    sound_handle_table = handles;
    handles[0] = &objects[0];
    handles[1] = &objects[1];
    sound_camera_x = 0;
    sound_camera_y = 0;
    sound_camera_z = 0;
    sound_emitter_scale = 0x10000;
    sound_voice_volume = 0;
    sound_voice_shift = 0;
    sound_voice_field = 0;
    sound_voice_flag = 0;
    basis_calls = 0;
    basis_argument = 0;
    event_calls = 0;
    event_basis = 0;
    event_emitter = 0;
}

static void set_emitter(int index, int object_index, int flags, int x, int y,
    int z, int basis0, int basis1, int basis2, int level, int field)
{
    emitters[index].object_index = object_index;
    emitters[index].flags = flags;
    emitters[index].x = x;
    emitters[index].y = y;
    emitters[index].z = z;
    emitters[index].basis0 = basis0;
    emitters[index].basis1 = basis1;
    emitters[index].basis2 = basis2;
    emitters[index].level = level;
    emitters[index].field_3c = field;
}

int main(void)
{
    int raw;

    reset();
    set_emitter(0, 0, 0, 100, 200, 300, 11, 22, 33, 4, 77);
    objects[0].field_10 = 0x2000;
    objects[0].field_14 = 3;
    sound_camera_x = 10;
    sound_camera_y = 20;
    sound_camera_z = 30;
    sound_emitter_scale = 0x8000;
    set_entry(0, 0);
    set_record_emitter(0, (int)&emitters[0]);
    sound_voice_setup();
    put(ctx_get(0xc8) == -1
        && ctx_get(0xd8) == (int)&emitters[0]
        && ctx_get(0xd4) == 0x30000
        && sound_voice_volume == 4
        && sound_voice_shift == 1
        && sound_voice_field == 77
        && ctx_get(0x30) == 11
        && ctx_get(0x34) == 22
        && ctx_get(0x38) == 33
        && ctx_get(0x3c) == 90
        && ctx_get(0x40) == 180
        && ctx_get(0x44) == 270
        && ctx_get(0x00) == (int)&objects[0]
        && ctx_get(0x04) == 3
        && ctx_get(0x08) == 0
        && basis_calls == 1
        && basis_argument == sound_voice_context + 0x30
        && event_calls == 0
        && sound_voice_flag == 0
        && sound_voice_entry == entry_list + 4 ? '0' : 'x');

    reset();
    set_emitter(1, 1, 0, 1, 2, 3, 4, 5, 6, 8, 9);
    objects[1].field_10 = 0x1000;
    objects[1].field_14 = 5;
    sound_emitter_scale = 0x4000;
    raw = (int)&emitters[1] | NEAR_FLAG;
    set_entry(0, 0x14);
    set_record_emitter(0x14, raw);
    sound_voice_setup();
    put(ctx_get(0xc8) == (int)&emitters[1] + 0x40
        && ctx_get(0xd8) == raw
        && ctx_get(0x30) == 4
        && ctx_get(0x34) == 5
        && ctx_get(0x38) == 6
        && ctx_get(0x3c) == 1
        && ctx_get(0x40) == 2
        && ctx_get(0x44) == 3
        && sound_voice_volume == 8
        && sound_voice_shift == 2
        && sound_voice_field == 9
        && ctx_get(0x00) == (int)&objects[1]
        && ctx_get(0x04) == 5
        && ctx_get(0x08) == 0
        && ctx_get(0xd4) == 0xc000
        && basis_calls == 1
        && event_calls == 0
        && sound_voice_flag == 0 ? '1' : 'x');

    reset();
    set_emitter(2, 0, 0x8000, 10, 20, 30, 7, 8, 9, -1, 0x123);
    objects[0].field_10 = -5;
    objects[0].field_14 = 2;
    sound_camera_x = 1;
    sound_camera_y = 2;
    sound_camera_z = 3;
    sound_emitter_scale = 0x6000;
    set_entry(0, 0);
    set_record_emitter(0, (int)&emitters[2]);
    sound_voice_setup();
    put(ctx_get(0xc8) == -1
        && ctx_get(0xd8) == (int)&emitters[2]
        && sound_voice_volume == 0x6000
        && sound_voice_shift == 0x2000
        && sound_voice_field == 0x123
        && ctx_get(0x3c) == 9
        && ctx_get(0x40) == 18
        && ctx_get(0x44) == 27
        && ctx_get(0x00) == (int)&objects[0]
        && ctx_get(0x04) == 2
        && ctx_get(0xd4) == -5
        && basis_calls == 0
        && event_calls == 1
        && event_basis == sound_voice_context + 0x30
        && event_emitter == &emitters[2]
        && sound_voice_flag == 1 ? '2' : 'x');

    reset();
    set_emitter(0, 0, 0, 40, 50, 60, 1, 2, 3, 6, 0x40);
    set_emitter(2, 1, 0x8000, 7, 8, 9, 4, 5, 6, -2, 0x50);
    objects[0].field_10 = 0x2000;
    objects[0].field_14 = 4;
    objects[1].field_10 = 0x2000;
    objects[1].field_14 = 6;
    sound_emitter_scale = 0x8000;
    set_entry(0, 0);
    set_entry(1, 0x14);
    set_record_emitter(0, (int)&emitters[0]);
    set_record_emitter(0x14, (int)&emitters[2]);
    sound_voice_setup();
    put(sound_voice_entry == entry_list + 4
        && basis_calls == 1
        && event_calls == 0
        && ctx_get(0x04) == 4 ? '3' : 'x');
    sound_voice_setup();
    put(sound_voice_entry == entry_list + 8
        && basis_calls == 1
        && event_calls == 1
        && ctx_get(0x04) == 6
        && ctx_get(0x00) == (int)&objects[1]
        && sound_voice_flag == 1 ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
