/* The rebuilt `map_object_finish` module.
 *
 * The original sits at linear 0x2b494 in STARTH.EXE and holds 118 instructions
 * and 457 bytes up to the `ret` at 0x2b65c.  The next function starts at
 * 0x2b660, so the store record keeps the 460-byte extent and sets no function
 * size.  The identity check uses 118 instructions.
 *
 * The only direct caller at 0x29b86 is the key -2 branch of `menu_text_update`
 * at 0x29918, which runs this function on each of the 0x50 map object records
 * at `DAT_001db78` and then restores the menu key.  The record writer
 * `map_object_update` at 0x2b9cc reads the same 0x64-byte records, so the
 * module records the name `map_object_finish`.  The name is inferred, because
 * the original holds no symbol for the function.
 *
 * A record with the field `+0x08` equal to -1 holds no object.  The function
 * binds the list name at 0x6350 to the record slot and returns.  Otherwise it
 * looks the object up in the slot table `DAT_001d2d0` and the 0x1c-byte entry
 * table `DAT_0010653c`, sets the object id scaled by `DAT_001fab8`, and copies
 * the three target positions `+0x40`, `+0x44`, and `+0x48` to the object
 * fields `+0x08`, `+0x0c`, and `+0x10`.
 *
 * When the record index `+0x04` equals the selected line
 * `DAT_001fb08[DAT_001fb60]`, the record angle `+0x10` takes pi/2.  The three
 * record angles `+0x10`, `+0x14`, and `+0x18` become the object angles `+0x14`,
 * `+0x18`, and `+0x1c`, scaled by 1024.0 / pi and masked with 0x7ff.  The
 * progress at `+0x58` and the timer at `+0x60` then reset to zero.
 *
 * The original calls the list helpers at 0x117fc and 0x11850 and the truncate
 * helper at 0x4d9f4.  This harness links no math library, so the local FPU stub
 * makes `__CHP` truncate toward zero.  The two float constants arrive through
 * globals, because the minimal image does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern void object_list_refresh(void *table, char *name, int *slot);
extern void object_list_bind(void *table, char *name, int *slot);

extern int DAT_001d2d0[];
extern int DAT_001fab8;
extern int DAT_001fb60;
extern int DAT_001fb08[];
extern char DAT_0010653c[];

extern float map_object_finish_angle_scale;
extern float map_object_finish_pi;

void map_object_finish(char *record)
{
    char *object;
    int index;
    int slot;

    if (*(int *)(record + 8) == -1) {
        object_list_bind((void *)0x6530, (char *)0x6350,
                         (int *)((char *)DAT_001d2d0
                             + *(int *)record * 8));
        return;
    }

    index = *(int *)record;
    slot = DAT_001d2d0[index * 2];
    object_list_refresh((void *)0x6530, (char *)0x6350,
                        (int *)((char *)DAT_001d2d0 + index * 8));
    object = *(char **)(DAT_0010653c + slot * 28);
    *(int *)(object + 0x20) =
        DAT_001fab8 * *(int *)(record + 8) + 0x226;
    *(int *)(object + 8) = *(int *)(record + 0x40);
    *(int *)(object + 0x0c) = *(int *)(record + 0x44);
    *(int *)(object + 0x10) = *(int *)(record + 0x48);
    if (*(int *)(record + 4) == DAT_001fb08[DAT_001fb60]) {
        *(int *)(record + 0x10) = 0x3fc90fdb;
    }
    *(int *)(object + 0x14) =
        (int)(*(float *)(record + 0x10) * map_object_finish_angle_scale
            / map_object_finish_pi) & 0x7ff;
    *(int *)(object + 0x18) =
        (int)(*(float *)(record + 0x14) * map_object_finish_angle_scale
            / map_object_finish_pi) & 0x7ff;
    *(int *)(object + 0x1c) =
        (int)(*(float *)(record + 0x18) * map_object_finish_angle_scale
            / map_object_finish_pi) & 0x7ff;
    *(int *)(record + 0x58) = 0;
    *(float *)(record + 0x60) = *(float *)(record + 0x58);
}
