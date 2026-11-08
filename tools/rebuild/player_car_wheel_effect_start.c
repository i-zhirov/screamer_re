/* The rebuilt `player_car_wheel_effect_start` module.
 *
 * The original sits at linear 0x19828 in STARTH.EXE and holds 99 instructions
 * and 344 bytes up to the `ret` at 0x1997f.  The next function starts at
 * 0x19ae8, so the store record keeps the 344-byte extent and sets no separate
 * function size.  The identity check uses 99 instructions.
 *
 * The two direct callers are the skid gate at 0x1810a in
 * player_car_skid_update at 0x17d14 and the reset step at 0x18737 in
 * player_car_reset_update at 0x18528.  Both callers run in the vehicle state
 * chain on the player object at object-3 offset 0xf91c after the player
 * physics group, so the module takes the section The vehicle state chain.
 *
 * The function starts the wheel effect slots.  It holds a rate timer at
 * +0x304.  A timer below 0.05 takes the frame delta DAT_00118e08 and returns.
 * Otherwise the timer clears and the function fills up to two of the eight
 * 0x28-byte effect slots at +0x308 through +0x448.
 *
 * The loop walks the slot index 0 and 2.  For each index it takes the first
 * free slot, sets the active flag at +0x08, resets the linked object frame
 * through the helper at 0x11974, refreshes the object list through the helper
 * at 0x117fc, and stores the slot position.
 *
 * The slot position starts at the two wheel offsets +0x18 and +0x20 of the
 * wheel unit at +0x194 with the index plus one.  The two odd units 1 and 3
 * are the rear wheels.  The function adds a dither: the low three bits of the
 * player position DAT_0010fdcc or DAT_0010fdd4 times the low six bits of the
 * frame counter DAT_000007fd.  The x dither takes the sign -1 when the frame
 * counter holds the bit of the loop index.
 *
 * The two callers, the four wheel units, and the two helper calls support the
 * name player_car_wheel_effect_start.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The 0.05 limit arrives through a global, because the minimal image does not
 * load the constant segment.  The module has no conversion to an integer, so
 * the shared fpu_stub.asm supplies the FPU run-time symbols.  The original has
 * the EBP frame, so the module builds with `-of+`.
 */
extern double player_car_wheel_effect_interval;

extern int DAT_000007fd;
extern int DAT_0010fdcc;
extern int DAT_0010fdd4;
extern float DAT_00118e08;

extern void object_frame_set(void *table, void *slot, float value);
extern void object_list_refresh(void *table, char *name, void *slot);

void player_car_wheel_effect_start(char *self)
{
    char *slot;
    char *end;
    int index;
    int direction;
    int offset_x;
    int offset_y;
    int counter;
    int pattern;

    slot = self + 0x308;
    end = self + 0x448;

    if ((double)*(float *)(self + 0x304) < player_car_wheel_effect_interval) {
        *(float *)(self + 0x304) =
            *(float *)(self + 0x304) + DAT_00118e08;
        return;
    }
    *(float *)(self + 0x304) = 0.0f;

    for (index = 0; index < 4; index += 2) {
        while (slot < end) {
            if (*(int *)(slot + 0x08) != 0) {
                slot = slot + 0x28;
                continue;
            }
            *(int *)(slot + 0x08) = 1;
            object_frame_set((void *)0x6530, slot, 0.0f);
            object_list_refresh((void *)0x6530, (char *)0x6350, slot);

            counter = DAT_000007fd;
            direction = (counter & index) ? -1 : 1;
            pattern = counter & 0x3f;
            offset_x = ((DAT_0010fdcc & 7) * pattern) * direction;
            offset_y = (DAT_0010fdd4 & 7) * pattern;

            *(int *)(slot + 0x18) =
                *(int *)(self + 0x1ac + (index + 1) * 0x54) + offset_x;
            *(int *)(slot + 0x20) =
                *(int *)(self + 0x1b4 + (index + 1) * 0x54) + offset_y;
            break;
        }
    }
}
