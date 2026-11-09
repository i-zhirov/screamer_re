/* The rebuilt `player_nearby_mark` module.
 *
 * The original sits at linear 0x12378 in STARTH.EXE and holds 57 instructions
 * and 170 bytes up to the `ret` at 0x12421.  The next record starts at 0x12424,
 * so the store record keeps the 172-byte extent and sets no separate function
 * size.  The identity check uses 57 instructions.
 *
 * The only direct caller at 0x15b28 is the player effect group at 0x15b08.
 * That group first calls the player group at 0x17c6c, then calls this function,
 * then runs `car_proximity_pass` at 0x17408 with the same player object in eax,
 * then runs `opponent_start` at 0x124c0.  The player update at 0x2003c runs the
 * effect group, so the record takes the section `The vehicle state chain`.
 *
 * The function clears the nearby mask at `+0x104`, then scans every 0x590-byte
 * car record at `DAT_00110ab8` from index 0 up to the count `DAT_0010f8ec`.
 * The player object holds its two position fields at `+0x4b0` and `+0x4b8`.
 * The car record holds the physics gate at `+0x414` and the two position
 * fields at `+0x498` and `+0x4a0`.  A record with a set gate whose position
 * stays within 1500 units of the player on both axes sets its index bit in the
 * mask.
 *
 * `car_proximity_pass` reads the mask at `+0x104`, so this function marks the
 * car records that the pass then tests.  The sibling marker `car_nearby_mark`
 * at 0x126e4 builds the per-car mask at `+0x0c` with the same gate and the same
 * 1500-unit bound.  The player scan, the mask, and the caller support the name
 * `player_nearby_mark`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
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

void player_nearby_mark(char *self)
{
    int index;
    char *other;

    *(int *)(self + 0x104) = 0;
    index = 0;
    other = (char *)DAT_00110ab8;
    while (index < DAT_0010f8ec) {
        if (*(int *)(other + 0x414) != 0
            && absolute(*(int *)(self + 0x4b0)
                - *(int *)(other + 0x498)) < 0x5dc
            && absolute(*(int *)(self + 0x4b8)
                - *(int *)(other + 0x4a0)) < 0x5dc) {
            *(int *)(self + 0x104) |= 1 << (index & 31);
        }
        other += 0x590;
        index = index + 1;
    }
}
