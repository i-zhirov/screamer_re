/* The rebuilt `car_setup` module.
 *
 * The original sits at linear 0x142ec in STARTH.EXE.  The plan row holds the
 * first 65 instructions and 226 bytes, up to the early `ret` at `0x143cd`.  The
 * full body continues to the `ret` at `0x1496c`, so the store record keeps the
 * 1668-byte extent to the next entry and holds no function size.
 *
 * The function stores the caller index at `+0` of the 0x590-byte car record.
 * When the load flag is zero, it returns 0.  Otherwise it clamps the global
 * slot counter at `DAT_0000f8f0` against the limit `DAT_0000d6dc`, skips the
 * reserved slot `DAT_0011d2a0`, and loads the `SHADOW` record into `+4` through
 * the record table at `0x6530`.  It selects the model index 0xc when the player
 * count `DAT_0011d2b4` is above 2 and the slot counter otherwise, stores the
 * index at `+0x2e0`, and loads the model record at `0xd25c + index * 0x40`
 * into `+0x4dc`.  A load error returns at once.
 *
 * The two callers at `0x12c0c` and `0x12f50` walk the car count
 * `DAT_0000f8ec` and call the function for each record at
 * `0x10ab8 + index * 0x590`.  The call target `0x11060` searches the
 * 0x78-record table at `0x6530` by name, registers a free slot, and loads the
 * object through `0x10bfc`.  The data at object-3 offset 0x5dc holds `SHADOW`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_0000f8f0;
extern int DAT_0000d6dc;
extern int DAT_0011d2a0;
extern int DAT_0011d2b4;
extern int object_load_named(void *table, char *name, int *slot);

int car_setup(void *object, int index, int load)
{
    int result;
    int choice;

    *(int *)object = index;
    if (load == 0) {
        return 0;
    }
    if (DAT_0000f8f0 >= DAT_0000d6dc) {
        DAT_0000f8f0 = 0;
    }
    if (DAT_0011d2a0 == DAT_0000f8f0) {
        DAT_0000f8f0 = DAT_0000f8f0 + 1;
    }
    if (DAT_0000d6dc <= DAT_0000f8f0) {
        DAT_0000f8f0 = 0;
    }
    result = object_load_named((void *)0x6530, (char *)0x5dc,
                               (int *)((char *)object + 4));
    if (result != 0) {
        return result;
    }
    if (DAT_0011d2b4 > 2) {
        choice = 0xc;
    } else {
        choice = DAT_0000f8f0;
    }
    *(int *)((char *)object + 0x2e0) = choice;
    result = object_load_named((void *)0x6530,
                               (char *)(0xd25c + (choice << 6)),
                               (int *)((char *)object + 0x4dc));
    if (result != 0) {
        return result;
    }
    return 0;
}
