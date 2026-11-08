/* The test harness for the rebuilt `player_car_surface_effect_update` module.
 *
 * The harness holds the player object, the four sound table entries, the car
 * index, the race mode, the effect scale, and the sound stub.
 *
 * The harness runs ten paths:
 *
 *     0  node value 0: the control word clears and the scale is 0.002; the
 *        previous value matches, so no sound starts
 *     1  node value 1: the control word is 0x8000 and the scale is 0.03; the
 *        previous value matches, so no sound starts
 *     2  node value 2: the control word is 0xff00 and the scale is 0.03; the
 *        previous value matches, so no sound starts
 *     3  node value 3: the switch keeps the control word and the scale, and
 *        the previous value matches, so no sound starts
 *     4  node value 1 after 0: the control word and the scale change, and the
 *        sound starts with the table entry 0
 *     5  race mode 0xc and car index 0: the sound uses the table entry 2
 *     6  race mode 0xc and car index 1: the sound uses the table entry 3
 *     7  state 1 after 0: the switch keeps the control word and the scale, and
 *        the sound still starts with the table entry 0
 *     8  car index 1: the sound uses the table entry 1
 *     D  the exit marker
 *
 * The scale arrives as its bit pattern, because the minimal image does not
 * load the constant segment.
 */
#pragma off (check_stack)

void player_car_surface_effect_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define OBJECT_SIZE 0x800

static char text[8];
static unsigned char object[OBJECT_SIZE];

int DAT_0011d2a0;
int DAT_0000ff88;
int DAT_00001d7c[8];
float DAT_000c26d8;

static int sound_calls;
static int sound_handle;
static int sound_sample;

void sound_sample_start(int handle, int sample)
{
    sound_calls++;
    sound_handle = handle;
    sound_sample = sample;
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

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

static void clear_bytes(unsigned char *p, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        p[index] = 0;
    }
}

static void set_object_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int object_int(int offset)
{
    return *(int *)(object + offset);
}

static void set_scale_bits(unsigned int bits)
{
    *(unsigned int *)&DAT_000c26d8 = bits;
}

static unsigned int scale_bits(void)
{
    return *(unsigned int *)&DAT_000c26d8;
}

static void reset(void)
{
    clear_bytes(object, OBJECT_SIZE);
    set_object_int(0x148, 0);
    set_object_int(0x58c, 0);
    set_object_int(0x66c, 0);
    set_object_int(0x7b0, 0x7eadbeef);
    set_object_int(0x7cc, 0x55);
    DAT_0011d2a0 = 0;
    DAT_0000ff88 = 0;
    set_scale_bits(0x3dcccccd);
    DAT_00001d7c[0] = 0x111;
    DAT_00001d7c[1] = 0x101;
    DAT_00001d7c[2] = 0x222;
    DAT_00001d7c[3] = 0x333;
    DAT_00001d7c[4] = 0x444;
    DAT_00001d7c[5] = 0x555;
    DAT_00001d7c[6] = 0x666;
    DAT_00001d7c[7] = 0x777;
    sound_calls = 0;
    sound_handle = 0;
    sound_sample = 0;
}

static int no_sound(void)
{
    return sound_calls == 0;
}

static int sound_is(int handle, int sample)
{
    return sound_calls == 1
        && sound_handle == handle
        && sound_sample == sample;
}

int main(void)
{
    int ok;

    reset();
    set_object_int(0x148, 0);
    set_object_int(0x66c, 0);
    set_object_int(0x58c, 0);
    player_car_surface_effect_update(object);
    ok = object_int(0x7b0) == 0
        && scale_bits() == 0x3b03126f
        && no_sound();
    report(ok, '0');

    reset();
    set_object_int(0x148, 0);
    set_object_int(0x66c, 1);
    set_object_int(0x58c, 1);
    player_car_surface_effect_update(object);
    ok = object_int(0x7b0) == 0x8000
        && scale_bits() == 0x3cf5c28f
        && no_sound();
    report(ok, '1');

    reset();
    set_object_int(0x148, 0);
    set_object_int(0x66c, 2);
    set_object_int(0x58c, 2);
    player_car_surface_effect_update(object);
    ok = object_int(0x7b0) == 0xff00
        && scale_bits() == 0x3cf5c28f
        && no_sound();
    report(ok, '2');

    reset();
    set_object_int(0x148, 0);
    set_object_int(0x66c, 3);
    set_object_int(0x58c, 3);
    player_car_surface_effect_update(object);
    ok = object_int(0x7b0) == 0x7eadbeef
        && scale_bits() == 0x3dcccccd
        && no_sound();
    report(ok, '3');

    reset();
    set_object_int(0x148, 0);
    set_object_int(0x66c, 1);
    set_object_int(0x58c, 0);
    player_car_surface_effect_update(object);
    ok = object_int(0x7b0) == 0x8000
        && scale_bits() == 0x3cf5c28f
        && sound_is(0x55, 0x111);
    report(ok, '4');

    reset();
    set_object_int(0x148, 0);
    set_object_int(0x66c, 1);
    set_object_int(0x58c, 0);
    DAT_0011d2a0 = 0xc;
    DAT_0000ff88 = 0;
    player_car_surface_effect_update(object);
    ok = sound_is(0x55, 0x222);
    report(ok, '5');

    reset();
    set_object_int(0x148, 0);
    set_object_int(0x66c, 1);
    set_object_int(0x58c, 0);
    DAT_0011d2a0 = 0xc;
    DAT_0000ff88 = 1;
    player_car_surface_effect_update(object);
    ok = sound_is(0x55, 0x333);
    report(ok, '6');

    reset();
    set_object_int(0x148, 1);
    set_object_int(0x66c, 2);
    set_object_int(0x58c, 0);
    player_car_surface_effect_update(object);
    ok = object_int(0x7b0) == 0x7eadbeef
        && scale_bits() == 0x3dcccccd
        && sound_is(0x55, 0x111);
    report(ok, '7');

    reset();
    set_object_int(0x148, 0);
    set_object_int(0x66c, 1);
    set_object_int(0x58c, 0);
    DAT_0000ff88 = 1;
    player_car_surface_effect_update(object);
    ok = sound_is(0x55, 0x101);
    report(ok, '8');

    put('D');
    quit();
    return 0;
}
