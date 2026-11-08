/* The rebuilt `player_car_contact_update` module.
 *
 * The original sits at linear 0x166a8 in STARTH.EXE and holds 159 instructions
 * and 524 bytes up to the `ret` at 0x168b3.  The next function starts at
 * 0x168b4, so the record keeps the 524-byte extent and sets no function size.
 *
 * The only direct caller at 0x15afc is the second player step group at
 * 0x15ab8.  The group runs on the player object at object-3 offset 0xf91c
 * after `player_car_cornering_update` at 0x16ca0, and the five view groups
 * call the group, so the module takes the section The vehicle state chain.
 *
 * The wheel contact step at 0x11a30 walks the same four 0x54-byte wheel units
 * at `+0x194`.  It stores the two ground contact heights at `+0x24` and
 * `+0x28` and the surface flag at `+0x2c`, and it writes the two contact
 * points at `+0x00`, `+0x08`, `+0x0c`, and `+0x14`.  The distances `+0x2e4`
 * and `+0x2ec` separate the wheels on the two axes.
 *
 * This function copies the two contact heights to `+0x04` and `+0x10`.  When
 * the surface flag is set, it adds a small signed step from the low bits of
 * the two contact coordinates to the first height.  It then writes the average
 * raw height `+0x4ec`, the average adjusted height `+0x4b4`, and the four body
 * angles `+0x4c8`, `+0x4d0`, `+0x4d4`, and `+0x4dc` through `atan` at
 * 0xc2254.  The body height, the pitch, and the roll feed the view orbit at
 * 0x1a3b8 and the view update at 0x16f34.
 *
 * The `atan` function comes from the local FPU assembly file, because the
 * harness links no math library.  The module converts no value to an integer,
 * so `__CHP` stays a bare return.  The original has the EBP frame, so the
 * module builds with `-of+`.
 */
extern double atan(double value);

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

static struct wheel_unit *unit_at(void *object, int index)
{
    return (struct wheel_unit *)((char *)object + 0x194 + index * 0x54);
}

void player_car_contact_update(void *object)
{
    int index;

    for (index = 0; index < 4; index++) {
        struct wheel_unit *unit = unit_at(object, index);

        unit->height_a = unit->ground_a;
        unit->height_b = unit->ground_b;
        if (unit->surface != 0) {
            int sum = unit->contact_x + unit->contact_z;
            int step = (sum & 2) ? -1 : 1;

            unit->height_a += step * (sum & 0xf);
        }
    }
    *(int *)((char *)object + 0x4ec) = (
        unit_at(object, 0)->ground_a
        + unit_at(object, 2)->ground_a
        + unit_at(object, 1)->ground_a
        + unit_at(object, 3)->ground_a)
        / 4;
    *(int *)((char *)object + 0x4b4) = (
        unit_at(object, 0)->height_a
        + unit_at(object, 2)->height_a
        + unit_at(object, 1)->height_a
        + unit_at(object, 3)->height_a)
        / 4;
    *(float *)((char *)object + 0x4d0) = (float)-atan(
        (double)(float)(unit_at(object, 0)->height_a
            - unit_at(object, 2)->height_a)
        / (double)*(int *)((char *)object + 0x2e4));
    *(float *)((char *)object + 0x4c8) = (float)atan(
        (double)(float)(unit_at(object, 2)->height_a
            - unit_at(object, 3)->height_a)
        / (double)*(int *)((char *)object + 0x2ec));
    *(float *)((char *)object + 0x4dc) = (float)-atan(
        (double)(float)(unit_at(object, 0)->height_b
            - unit_at(object, 2)->height_b)
        / (double)*(int *)((char *)object + 0x2e4));
    *(float *)((char *)object + 0x4d4) = (float)atan(
        (double)(float)(unit_at(object, 2)->height_b
            - unit_at(object, 3)->height_b)
        / (double)*(int *)((char *)object + 0x2ec));
}
