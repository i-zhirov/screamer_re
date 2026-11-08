/* The rebuilt `sla_marker_check` module.
 *
 * The original sits at linear 0x18ba8 in STARTH.EXE and holds 90 instructions
 * and 291 bytes up to the `ret` at 0x18cca.  The next function starts at
 * 0x18ccc, so the store record keeps the 292-byte extent and sets no separate
 * function size.  The identity check uses 90 instructions.
 *
 * The only direct caller at 0x203de is the view group at 0x203c4.  That group
 * runs on the player object at object-3 offset 0xf91c after
 * `player_car_suspension_update` at 0x168b4, and it calls this function only
 * when bit 0 of `DAT_00118e1d` is set and bit 0x40 of the game mode
 * `DAT_0011d260` is set.  The game mode 0x40 is the SLA mode, and the loader
 * at 0x150f4 fills the marker table at 0x7890 from the `%ssla` member only for
 * that mode.  The module therefore takes the section `The vehicle state chain`.
 *
 * The function walks the 0x20-byte SLA markers at `sla_marker_table` while the
 * index stays below `sla_marker_count`.  A marker holds the radius at `+0x00`,
 * the value at `+0x10`, the x position at `+0x14`, and the y position at
 * `+0x1c`.
 *
 * A marker is out of range when the absolute x delta or the absolute y delta
 * is at or above the radius, or when the square root of the summed squares is
 * above the radius.  An out-of-range marker advances the search counter at
 * `+0x75c` and the next marker.
 *
 * The first marker in range ends the scan.  When its flag in the array at
 * `+0x694` is set, the function returns at once.  Otherwise it sets the flag,
 * increments the hit count at `+0x7c0`, adds the marker value to the score
 * float at `+0x790`, and calls `table_lookup` with 9.
 *
 * The original calls the square root at 0x22bec and the table lookup at
 * 0x1f718.  The module converts no float to an integer, so the local
 * `sla_marker_check_fpu.asm` supplies only the square root and the FPU
 * run-time symbols.  The original has the EBP frame, so the module builds with
 * `-of+`.
 */
extern int sla_marker_count;
extern int table_lookup(int index);
extern double sqrt(double value);

extern struct sla_marker {
    int radius;
    int reserved04;
    int reserved08;
    int reserved0c;
    int value;
    int x;
    int reserved18;
    int y;
} sla_marker_table[];

void sla_marker_check(void *object)
{
    struct sla_marker *entry;
    struct sla_marker *end;
    int index;

    entry = sla_marker_table;
    end = sla_marker_table + sla_marker_count;
    *(int *)((char *)object + 0x75c) = 0;
    while (entry < end) {
        int dx;
        int dy;
        int distance;

        dx = *(int *)((char *)object + 0x4b0) - entry->x;
        if (dx < 0) {
            dx = -dx;
        }
        if (dx >= entry->radius) {
            *(int *)((char *)object + 0x75c) =
                *(int *)((char *)object + 0x75c) + 1;
            entry++;
            continue;
        }
        dy = *(int *)((char *)object + 0x4b8) - entry->y;
        if (dy < 0) {
            dy = -dy;
        }
        if (dy >= entry->radius) {
            *(int *)((char *)object + 0x75c) =
                *(int *)((char *)object + 0x75c) + 1;
            entry++;
            continue;
        }
        dx = *(int *)((char *)object + 0x4b0) - entry->x;
        dy = *(int *)((char *)object + 0x4b8) - entry->y;
        distance = dx * dx + dy * dy;
        if ((double)entry->radius < sqrt((double)distance)) {
            *(int *)((char *)object + 0x75c) =
                *(int *)((char *)object + 0x75c) + 1;
            entry++;
            continue;
        }
        index = *(int *)((char *)object + 0x75c);
        if (*(int *)((char *)object + 0x694 + index * 4) != 0) {
            return;
        }
        *(int *)((char *)object + 0x694 + index * 4) = 1;
        *(int *)((char *)object + 0x7c0) =
            *(int *)((char *)object + 0x7c0) + 1;
        *(float *)((char *)object + 0x790) =
            *(float *)((char *)object + 0x790) + (float)entry->value;
        table_lookup(9);
        return;
    }
}
