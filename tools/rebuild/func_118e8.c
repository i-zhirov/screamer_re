/* The rebuilt function at linear 0x118e8 in STARTH.EXE.
 *
 * The original body holds 46 instructions and 139 bytes up to the `ret 4` at
 * 0x11970.  The next function starts at 0x11974 after the one padding byte at
 * 0x11973, so the store record keeps the 140-byte extent and sets no separate
 * function size.  The identity check uses 46 instructions.
 *
 * The only direct caller is at 0x19a77 in player_car_wheel_effect_update at
 * 0x19980.  The caller passes the object list at 0x6530 in eax, the effect
 * slot in edx, and the frame delta on the stack.  The effect group runs the
 * caller on the player object, so the record takes the section The vehicle
 * state chain.
 *
 * The function reads the object index from the first int of the effect slot.
 * The index selects one 0x1c-byte entry of the object list.  A zero file
 * record at entry + 0x14 marks a free entry, and the function returns 0x11.
 * The block object at entry + 0x0c holds the frame counter at + 0x20.
 *
 * The function adds the delta times 65535.0 to the frame counter and truncates
 * the sum toward zero.  The scale maps a normalized delta onto the 16-bit
 * frame units.  A sum below 0x10000 stores the sum and returns 0.  A sum at or
 * above 0x10000 stores the sum minus 0x10000 and returns -1, so the caller
 * releases the effect slot.
 *
 * The scale arrives through a global, because the original reads the float at
 * object-3 offset 0x3e8 (linear 0x1003e8).  The original calls the truncating
 * helper at 0x4d9f4, so the local FPU stub makes __CHP truncate toward zero.
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern float func_118e8_scale;

int func_118e8(void *table, int *slot, float delta)
{
    char *entry;
    char *object;
    int frame;

    entry = (char *)table + *slot * 0x1c;
    if (*(int *)(entry + 0x14) == 0) {
        return 0x11;
    }

    object = *(char **)(entry + 0x0c);
    frame = *(int *)(object + 0x20);
    frame = (int)(frame + delta * func_118e8_scale);
    *(int *)(object + 0x20) = frame;

    if (frame >= 0x10000) {
        *(int *)(object + 0x20) = frame - 0x10000;
        return -1;
    }
    return 0;
}
