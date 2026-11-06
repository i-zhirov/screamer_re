/* The test harness for the rebuilt `player_car_contact_update` module.
 *
 * The harness holds the player object and the four wheel units.  The `atan`
 * stub lives in the local FPU assembly file.  The harness runs five paths:
 *
 *     0  the surface flag is clear: the two contact heights copy to the two
 *        body heights, the averages are 22, and the four angles are pi/4 or
 *        -pi/4
 *     1  the surface flag is set and the low bits give a positive step: the
 *        adjusted height is 105 and the raw height is 100, and the zero angles
 *        are positive or negative zero
 *     2  the surface flag is set and bit 1 gives a negative step: the adjusted
 *        height is 47 and the raw height is 50
 *     3  only two units have the surface flag: the gates keep the heights, the
 *        averages are 8 and 9, and the negative zero angles are -0.0
 *     4  the negative heights divide toward zero: the averages are -2, and the
 *        four angles are pi/4 or -pi/4
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.  The float comparisons use
 * the raw bits, because the constant pool of this toolchain reads at a low
 * address.
 */
#pragma off (check_stack)

struct wheel_unit {
    int contact_x;
    int height_a;
    int contact_z;
    unsigned char pad0c[4];
    int height_b;
    unsigned char pad14[0x10];
    int ground_a;
    int ground_b;
    int surface;
    unsigned char pad30[0x24];
};

void player_car_contact_update(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static unsigned char object[0x900];
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

static void set_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int get_int(int offset)
{
    return *(int *)(object + offset);
}

static unsigned int get_bits(int offset)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.f = *(float *)(object + offset);
    return v.u;
}

static struct wheel_unit *unit_at(int index)
{
    return (struct wheel_unit *)(object + 0x194 + index * 0x54);
}

static void clear_object(void)
{
    int index;

    for (index = 0; index < 0x900; index++) {
        object[index] = 0;
    }
}

static void set_unit(int index, int contact_x, int contact_z, int ground_a,
                     int ground_b, int surface)
{
    struct wheel_unit *unit = unit_at(index);

    unit->contact_x = contact_x;
    unit->contact_z = contact_z;
    unit->ground_a = ground_a;
    unit->ground_b = ground_b;
    unit->surface = surface;
}

int main(void)
{
    int index;

    clear_object();
    set_int(0x2e4, 10);
    set_int(0x2ec, 10);
    set_unit(0, 0, 0, 10, 10, 0);
    set_unit(1, 0, 0, 30, 0, 0);
    set_unit(2, 0, 0, 20, 20, 0);
    set_unit(3, 0, 0, 30, 30, 0);
    player_car_contact_update(object);
    put(unit_at(0)->height_a == 10 && unit_at(1)->height_a == 30
            && unit_at(2)->height_a == 20 && unit_at(3)->height_a == 30
            && unit_at(0)->height_b == 10 && unit_at(2)->height_b == 20
            && get_int(0x4ec) == 22 && get_int(0x4b4) == 22
            && get_bits(0x4d0) == 0x3f490fdb
            && get_bits(0x4c8) == 0xbf490fdb
            && get_bits(0x4dc) == 0x3f490fdb
            && get_bits(0x4d4) == 0xbf490fdb
        ? '0' : 'x');

    clear_object();
    set_int(0x2e4, 1);
    set_int(0x2ec, 1);
    for (index = 0; index < 4; index++) {
        set_unit(index, 5, 0, 100, 0, 1);
    }
    player_car_contact_update(object);
    put(unit_at(0)->height_a == 105 && unit_at(1)->height_a == 105
            && unit_at(2)->height_a == 105 && unit_at(3)->height_a == 105
            && unit_at(1)->height_b == 0
            && get_int(0x4ec) == 100 && get_int(0x4b4) == 105
            && get_bits(0x4d0) == 0x80000000 && get_bits(0x4c8) == 0
            && get_bits(0x4dc) == 0x80000000 && get_bits(0x4d4) == 0
        ? '1' : 'x');

    clear_object();
    set_int(0x2e4, 1);
    set_int(0x2ec, 1);
    for (index = 0; index < 4; index++) {
        set_unit(index, 0x13, 0, 50, 7, 1);
    }
    player_car_contact_update(object);
    put(unit_at(0)->height_a == 47 && unit_at(3)->height_a == 47
            && unit_at(0)->height_b == 7
            && get_int(0x4ec) == 50 && get_int(0x4b4) == 47
            && get_bits(0x4d0) == 0x80000000 && get_bits(0x4d4) == 0
        ? '2' : 'x');

    clear_object();
    set_int(0x2e4, 1);
    set_int(0x2ec, 4);
    set_unit(0, 0xf, 0, 8, 3, 0);
    set_unit(1, 1, 0, 8, 0, 1);
    set_unit(2, 2, 0, 8, 3, 0);
    set_unit(3, 4, 0, 8, 7, 1);
    player_car_contact_update(object);
    put(unit_at(0)->height_a == 8 && unit_at(1)->height_a == 9
            && unit_at(2)->height_a == 8 && unit_at(3)->height_a == 12
            && unit_at(0)->height_b == 3 && unit_at(3)->height_b == 7
            && get_int(0x4ec) == 8 && get_int(0x4b4) == 9
            && get_bits(0x4d0) == 0x80000000 && get_bits(0x4c8) == 0xbf490fdb
            && get_bits(0x4dc) == 0x80000000 && get_bits(0x4d4) == 0xbf490fdb
        ? '3' : 'x');

    clear_object();
    set_int(0x2e4, 2);
    set_int(0x2ec, 1);
    set_unit(0, 0, 0, -1, 0, 0);
    set_unit(1, 0, 0, -2, 0, 0);
    set_unit(2, 0, 0, -3, -2, 0);
    set_unit(3, 0, 0, -4, -3, 0);
    player_car_contact_update(object);
    put(unit_at(0)->height_a == -1 && unit_at(3)->height_a == -4
            && get_int(0x4ec) == -2 && get_int(0x4b4) == -2
            && get_bits(0x4d0) == 0xbf490fdb
            && get_bits(0x4c8) == 0x3f490fdb
            && get_bits(0x4dc) == 0xbf490fdb
            && get_bits(0x4d4) == 0x3f490fdb
        ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
