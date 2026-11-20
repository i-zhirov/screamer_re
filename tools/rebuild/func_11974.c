/* The rebuilt function at linear 0x11974 in STARTH.EXE.
 *
 * The original body holds 28 instructions and 85 bytes up to the `ret 4` at
 * 0x119c6.  The next entry starts at 0x119cc, so the store record keeps the
 * 88-byte extent and sets no separate function size.  The identity check uses
 * 28 instructions.
 *
 * The only direct caller is at 0x198b0 in player_car_wheel_effect_start at
 * 0x19828.  The caller passes the object list at 0x6530 in eax, the effect slot
 * in edx, and the float 0.0 on the stack.  The caller runs in the vehicle state
 * chain on the player object at object-3 offset 0xf91c, so the record takes the
 * section The vehicle state chain.
 *
 * The function reads the object index from the first int of the effect slot.
 * The index selects one 0x1c-byte entry of the object list.  A zero file
 * record at entry + 0x14 marks a free entry, and the function returns 0x11.
 * The block object pointer sits at entry + 0x0c, and its frame counter sits at
 * object + 0x20.
 *
 * The function sets the frame counter to the delta times 65535.0, truncated
 * toward zero.  The scale maps a normalized delta onto the 16-bit frame units.
 * It sets the counter in place, so a zero delta clears it.  The caller passes
 * 0.0, so it resets the frame counter at the effect start.
 *
 * The scale arrives through a global, because the original reads the float at
 * object-3 offset 0x3ec (linear 0x1003ec).  The original calls the truncating
 * helper at 0x4d9f4, so the local FPU stub makes __CHP truncate toward zero.
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern float func_11974_scale;

int func_11974(void *table, int *slot, float delta)
{
    char *entry;
    char *object;
    int frame;

    entry = (char *)table + *slot * 0x1c;
    if (*(int *)(entry + 0x14) == 0) {
        return 0x11;
    }

    object = *(char **)(entry + 0x0c);
    frame = (int)(delta * func_11974_scale);
    *(int *)(object + 0x20) = frame;

    return 0;
}
