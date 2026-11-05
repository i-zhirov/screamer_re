/* The rebuilt `player_car_init` module.
 *
 * The original sits at linear 0x136cc in STARTH.EXE.  The plan row holds the
 * first 87 instructions and 277 bytes up to the early `ret` at `0x137e0`.
 * The full body continues to the `ret` at `0x142ea`, so the store record keeps
 * the 3104-byte extent to the next entry and holds no function size.
 *
 * The function consumes the selected 0x40-byte `CARS_DATA` record.  The record
 * table at object-3 offset 0xd25c holds one record for each car, and the global
 * index `DAT_0011d2a0` selects one record.  The record layout is:
 *
 *     +0x00    char filename[0xc]
 *     +0x0c    int  animation
 *     +0x10    float maxspd, maxrpm, grip, accel
 *     +0x20    int  shiftmode, n_shifts
 *     +0x28    float sh_rap0 .. sh_rap5
 *
 * The function copies the record to the player object at `+0x470`.  The cockpit
 * name uses the last two characters of the file name, so `AUTO00` gives `00`.
 * The animation field is a flag: zero gives the letter `A` and a non-zero value
 * gives the letter `B`.  The function builds `COCK%s%c` and loads the cockpit
 * resource through `0x1b23c`, and it builds the same name with the letter `H`
 * and reads the `CARS` list of `GEN.dat` through `0x15460`.
 *
 * The function then loads the `SHADOW` object into `+0x468` and the animation
 * object named by the record into `+0x460` through `0x11060`.  A load error
 * returns its code at once.
 *
 * The rebuild covers the 87-instruction prefix.  The zero load flag returns 0,
 * and the success path returns 0 after the two object loads.  The state reset
 * after `0x13dcf` is not in the module.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern char DAT_0000d25c[];
extern int DAT_0011d2a0;
extern int sprintf(char *buffer, char *format, ...);
extern void cockpit_resource_load(char *name, void **out);
extern int list_file_load(char *type, char *name, char *list, void *table,
                          int stride, int count);
extern int object_load_named(void *table, char *name, int *slot);

typedef struct car_record {
    char filename[0xc];
    int animation;
    char values[0x30];
} car_record;

typedef struct player_car {
    char reserved_00[0x460];
    int animation_object;
    int reserved_464;
    int shadow_object;
    int reserved_46c;
    car_record record;
} player_car;

int player_car_init(player_car *car, int load)
{
    char name[0x58];
    int result;
    int suffix;

    if (load == 0) {
        return 0;
    }
    car->record = *(car_record *)(DAT_0000d25c + DAT_0011d2a0 * 0x40);
    suffix = car->record.animation != 0 ? 'B' : 'A';
    sprintf(name, (char *)0x564, car->record.filename + 4, suffix);
    cockpit_resource_load(name, (void **)0x18f0c);
    sprintf(name, (char *)0x56d, car->record.filename + 4, 'H');
    list_file_load((char *)0x57b, (char *)0x576, name, (void *)0x18f1c,
                   0x74, 0x74);
    result = object_load_named((void *)0x6530, (char *)0x57f,
                               &car->shadow_object);
    if (result != 0) {
        return result;
    }
    result = object_load_named((void *)0x6530, car->record.filename,
                               &car->animation_object);
    if (result != 0) {
        return result;
    }
    return 0;
}
