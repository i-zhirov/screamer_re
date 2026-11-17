/* The rebuilt `traffic_nearby_mark` module.
 *
 * The original sits at linear 0x12424 in STARTH.EXE and holds 56 instructions
 * and 155 bytes up to the `ret` at 0x124be.  The next function starts at
 * 0x124c0, so the store record keeps the 156-byte extent and sets no separate
 * function size.  The identity check uses 56 instructions.
 *
 * The only direct caller at 0x15b64 is the traffic effect group at 0x15b44.
 * That group first calls this function, then the traffic collision pass at
 * 0x175dc, then the opponent start at 0x124c0.  The traffic view group at
 * 0x201d0 runs the effect group at 0x20268, so the record takes the section
 * `The vehicle state chain`.
 *
 * The function clears the nearby mask at `+0x104` and then scans the
 * 0x2a8-byte traffic records at `DAT_00113cd8`.  The original loop bound is the
 * fixed count 8.  A record with a negative first field is inactive, so the
 * function skips it.  The player object holds its two position fields at
 * `+0x4b0` and `+0x4b8`.  The traffic record holds its two position fields at
 * `+0x3c` and `+0x44`.  A record within 1500 units of the player on both axes
 * sets its index bit in the mask.
 *
 * `traffic_collision_pass` reads the mask at `+0x104`, so this function marks
 * the traffic records that the pass then tests.  The sibling markers
 * `player_nearby_mark` at 0x12378 and `car_nearby_mark` at 0x126e4 build the
 * same kind of mask for the car records.  The traffic table, the mask, and the
 * caller support the name `traffic_nearby_mark`.  The original has no symbol,
 * so the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no direct call, so it needs
 * no FPU stub and no assembly.
 */
extern unsigned char DAT_00113cd8[];

static int absolute(int value)
{
    return value < 0 ? -value : value;
}

void traffic_nearby_mark(char *self)
{
    int index;
    char *record;

    *(int *)(self + 0x104) = 0;
    index = 0;
    record = (char *)DAT_00113cd8;
    while (index < 8) {
        if (*(int *)record >= 0
            && absolute(*(int *)(self + 0x4b0)
                - *(int *)(record + 0x3c)) < 0x5dc
            && absolute(*(int *)(self + 0x4b8)
                - *(int *)(record + 0x44)) < 0x5dc) {
            *(int *)(self + 0x104) |= 1 << (index & 31);
        }
        record += 0x2a8;
        index = index + 1;
    }
}
