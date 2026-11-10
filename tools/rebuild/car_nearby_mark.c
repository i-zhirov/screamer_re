/* The rebuilt `car_nearby_mark` module.
 *
 * The original sits at linear 0x126e4 in STARTH.EXE and holds 60 instructions
 * and 178 bytes up to the `ret` at 0x12795.  The next function starts at
 * 0x127a0, so the store record keeps the 188-byte extent and sets no function
 * size.  The identity check uses 60 instructions.
 *
 * The only direct caller at 0x1c7f4 is the unnamed per-object step at 0x1c7d0.
 * That step first calls this function, then runs `car_collision_pass` at
 * 0x1d00c, then runs `opponent_avoid_player` at 0x1e414.  The object update at
 * 0x2003c runs the step on each 0x590-byte car record at `DAT_00110ab8` after
 * `object_prepare`.  The record therefore takes the section `object update
 * callees`.
 *
 * The function clears the mask at `+0x0c`, reads the record number at `+0x00`,
 * and scans every later car record up to the count `DAT_0010f8ec`.  For each
 * candidate the physics gate at `+0x414` must be set, and the two position
 * fields at `+0x498` and `+0x4a0` must stay within 1500 units of the record on
 * both axes.  A nearby candidate sets its index bit in the mask.
 *
 * `car_collision_pass` reads the same mask at `+0x0c`, so this function marks
 * the later car records that the collision pass then tests.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no direct call, so it needs
 * no FPU stub and no assembly.
 */
extern unsigned char DAT_00110ab8[];
extern int DAT_0010f8ec;

static int absolute(int value)
{
    return value < 0 ? -value : value;
}

void car_nearby_mark(char *self)
{
    int index;
    char *other;

    *(int *)(self + 0xc) = 0;
    index = *(int *)self + 1;
    other = (char *)DAT_00110ab8 + index * 0x590;
    while (index < DAT_0010f8ec) {
        if (*(int *)(other + 0x414) != 0
            && absolute(*(int *)(self + 0x498)
                - *(int *)(other + 0x498)) < 0x5dc
            && absolute(*(int *)(self + 0x4a0)
                - *(int *)(other + 0x4a0)) < 0x5dc) {
            *(int *)(self + 0xc) |= 1 << (index & 31);
        }
        other += 0x590;
        index = index + 1;
    }
}
