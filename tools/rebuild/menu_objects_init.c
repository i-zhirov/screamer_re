/* The rebuilt `menu_objects_init` module.
 *
 * The original sits at linear 0x2aab8 in STARTH.EXE and holds 395 instructions
 * and 1420 bytes up to the `ret` at 0x2b043, so the record keeps the 1420-byte
 * extent and sets no function size.
 *
 * The only caller at 0x29f0c is `menu_space_init` at 0x29d08.  The function
 * loads the menu object list and fills the car, map, and member tables, so the
 * module records the name `menu_objects_init`.
 *
 * The function first copies the 20-byte names `AAUTO00` from `DAT_00002edc` and
 * `AMAP1` from `DAT_00002ef0` to local buffers.  It loads the `OBJ_LIST`
 * resource through 0x119cc, then looks the object `ALETTER` up in the list at
 * `0x6530`.  Each failure runs `bios_video(3)`, `error_report`, and `exit` and
 * then continues, because the harness stubs return.
 *
 * The loop at 0x2ab18 loads 79 objects into the 8-byte slots at `DAT_001d2d0`.
 * The loop at 0x2ab4a fills the 0xe slots at `DAT_001d550` with the names
 * `AAUTO00` through `AAUTO06`, because the byte at offset 6 comes from the
 * counter.  Each slot takes two loads.  The first load sets the object field
 * `+0x20` to 0x6000 and `+0x3c` to 0; the second load sets `+0x20` to 0xe000
 * and `+0x3c` to 0.
 *
 * The loops at 0x2ac96, 0x2ad62, and 0x2ae34 fill the 0xc slots at
 * `DAT_001d908` with `AMAP1` through `AMAP6` twice and bind the object slots
 * through 0x11850.  The cardat list at `DAT_001fd10` supplies 14 names and five
 * numbers each.  The trackdat list at `DAT_001fea0` supplies 12 names and one
 * number each.  The last call at 0x2b037 runs the init at 0x2dff4.
 *
 * The call target 0x11060 searches the object list at 0x6530 by name and
 * registers a free slot.  The target 0x10ed4 loads the next object into an
 * 8-byte slot.  The target 0x11568 links the object list, and the target
 * 0x11850 binds one slot.  The data at object-3 offset 0x3f0 holds `OBJ_LIST`,
 * at 0x3ca `3D_BLKOBJ`, at 0x108f `ALETTER`, at 0x2edc `AAUTO00`, and at
 * 0x2ef0 `AMAP1`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern char DAT_00002edc[];
extern char DAT_00002ef0[];
extern char DAT_0000108f[];
extern int DAT_001d2d0[];
extern int DAT_001d550[];
extern int DAT_001d908[];
extern int DAT_001fd10[];
extern int DAT_001fea0[];
extern int DAT_00006530[];

extern void sub_119cc(void);
extern int object_load_named(void *table, char *name, int *slot);
extern int object_load_next(void *table, void *slot, void *start);
extern void bios_video(int mode);
extern void error_report(int code);
extern void exit(int code);
extern void object_list_link(char *first, char *second);
extern int atoi(char *text);
extern void object_list_bind(void *table, char *name, void *slot);
extern void sub_2dff4(void);

static void copy_bytes(char *target, char *source, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        target[index] = source[index];
    }
}

static void copy_text(char *target, char *source)
{
    while ((*target = *source) != 0) {
        target = target + 1;
        source = source + 1;
    }
}

static char *object_at(int index)
{
    return *(char **)((char *)DAT_00006530 + index * 0x1c + 0xc);
}

static void set_object(int index, int field_20)
{
    char *object;

    object = object_at(index);
    *(int *)(object + 0x20) = field_20;
    *(int *)(object + 0x3c) = 0;
}

void menu_objects_init(void)
{
    char cars[20];
    char maps[20];
    int index;
    int counter;
    int result;
    char *slot;

    copy_bytes(cars, DAT_00002edc, 20);
    copy_bytes(maps, DAT_00002ef0, 20);

    sub_119cc();

    result = object_load_named((void *)0x6530, DAT_0000108f,
                               (int *)DAT_001d2d0);
    if (result != 0) {
        bios_video(3);
        error_report(result);
        exit(result);
    }

    for (index = 1; index < 0x50; index++) {
        object_load_next((void *)0x6530,
                         (char *)DAT_001d2d0 + index * 8,
                         (void *)DAT_001d2d0);
    }

    for (index = 0; index < 0xe; index = index + 2) {
        result = object_load_named((void *)0x6530, cars,
                                   (int *)((char *)DAT_001d550 + index * 0x44));
        if (result != 0) {
            bios_video(3);
            error_report(result);
            exit(result);
        }
        result = object_load_named((void *)0x6530, cars,
                                   (int *)((char *)DAT_001d550
                                       + (index + 1) * 0x44));
        if (result != 0) {
            bios_video(3);
            error_report(result);
            exit(result);
        }
        set_object(*(int *)((char *)DAT_001d550 + index * 0x44), 0x6000);
        set_object(*(int *)((char *)DAT_001d550 + (index + 1) * 0x44), 0xe000);
        cars[6] = cars[6] + 1;
    }

    for (index = 0; index < 6; index++) {
        result = object_load_named((void *)0x6530, maps,
                                   (int *)((char *)DAT_001d908 + index * 0x44));
        if (result != 0) {
            bios_video(3);
            error_report(result);
            exit(result);
        }
        set_object(*(int *)((char *)DAT_001d908 + index * 0x44), 0x6000);
        maps[4] = maps[4] + 1;
    }

    maps[4] = 0x31;
    for (index = 6; index < 0xc; index++) {
        result = object_load_named((void *)0x6530, maps,
                                   (int *)((char *)DAT_001d908 + index * 0x44));
        if (result != 0) {
            bios_video(3);
            error_report(result);
            exit(result);
        }
        set_object(*(int *)((char *)DAT_001d908 + index * 0x44), 0x6000);
        maps[4] = maps[4] + 1;
    }

    object_list_link((char *)0x6530, (char *)0x6350);
    counter = 0;
    for (index = 0; index < 0xe; index++) {
        slot = (char *)DAT_001d550 + index * 0x44;
        copy_text(slot + 0x14, (char *)DAT_001fd10[counter]);
        counter = counter + 1;
        *(int *)(slot + 0x30) = atoi((char *)DAT_001fd10[counter]);
        counter = counter + 1;
        *(int *)(slot + 0x34) = atoi((char *)DAT_001fd10[counter]);
        counter = counter + 1;
        *(int *)(slot + 0x38) = atoi((char *)DAT_001fd10[counter]);
        counter = counter + 1;
        *(int *)(slot + 0x3c) = atoi((char *)DAT_001fd10[counter]);
        counter = counter + 1;
        *(int *)(slot + 0x40) = atoi((char *)DAT_001fd10[counter]);
        counter = counter + 1;
        object_list_bind((void *)0x6530, (char *)0x6350, slot);
    }

    counter = 0;
    for (index = 0; index < 0xc; index++) {
        slot = (char *)DAT_001d908 + index * 0x44;
        copy_text(slot + 0x14, (char *)DAT_001fea0[counter]);
        counter = counter + 3;
        *(int *)(slot + 0x30) = atoi((char *)DAT_001fea0[counter]);
        counter = counter + 1;
        object_list_bind((void *)0x6530, (char *)0x6350, slot);
    }

    sub_2dff4();
}
