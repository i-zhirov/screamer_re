/* The rebuilt `game_mode_100_start` module.
 *
 * The original sits at linear 0x12f50 in STARTH.EXE and holds 85 instructions
 * and 335 bytes up to the `ret` at 0x1309e.  The next function starts at
 * 0x130a0, so the store record keeps the 336-byte extent and sets no separate
 * function size.  The identity check uses 85 instructions.
 *
 * The only direct caller is the game mode dispatch at 0x12996 in
 * race_session_start at 0x128e0, which sends the mode 0x100 to this function.
 * The caller takes the section The timing and filter layer, so this module
 * takes the same section.  The caller and the program map already hold the
 * name game_mode_100_start with the confidence inferred.
 *
 * The function starts the mode 0x100 scene.  It clears the credits session
 * state DAT_0011ce44, the timer DAT_0011ce48, the toggle DAT_0011ce40, the
 * line index DAT_0011ce3c, the two roll cells DAT_0011d220 and DAT_0011d21c,
 * and the panel cell DAT_0011ce50.  It sets the panel position
 * DAT_0011ce4c to -320.0 and the fade level DAT_0011d224 to 1.0 through their
 * bit patterns.  It clears the car slot counter DAT_0000f8f0 and fills the
 * twenty credit line positions DAT_0011ce58 with 0x15e and the twenty cells
 * DAT_0011cea8 with 0x64.
 *
 * The car loop walks the car count DAT_0000f8ec.  For each 0x590-byte car
 * record at DAT_00010ab8 it calls car_setup with the record, the index, and
 * the load flag 1.  A non-zero result returns at once.
 *
 * After the car loop the function loads the `attr` member at object-3 offset
 * 0x47b into the credit page table DAT_0011cef8 through member_list_load and
 * stores the result at DAT_0011d218.  It then sets the frame flags: the two
 * cells DAT_0010a9c and DAT_00001e04 to 1, DAT_0010a80 and DAT_0010a1c to -1,
 * DAT_0010a18 and DAT_0010a94 to 0, bit 0x2001 of the word DAT_00118e1c, and
 * the frame counter DAT_000007fd at DAT_0011d228.  It clears bit 1 of the high
 * byte DAT_00118e1d.  Finally it runs the scene setup at 0x15984 with the mode
 * argument and returns 0.
 *
 * The minimal harness image does not load the constant segment, so the two
 * float cells take their bit patterns as integers.  The original has the EBP
 * frame, so the module builds with `-of+`.
 */
typedef union {
    unsigned short bits;
    unsigned char bytes[2];
} control_flags;

extern int DAT_0011ce3c;
extern int DAT_0011ce40;
extern int DAT_0011ce44;
extern unsigned int DAT_0011ce48;
extern unsigned int DAT_0011ce4c;
extern int DAT_0011ce50;
extern int DAT_0011ce58[];
extern int DAT_0011cea8[];
extern char *DAT_0011cef8[];
extern int DAT_0011d218;
extern int DAT_0011d21c;
extern int DAT_0011d220;
extern unsigned int DAT_0011d224;
extern int DAT_0011d228;
extern control_flags DAT_00118e1c;
extern int DAT_00001e04;
extern int DAT_000007fd;
extern int DAT_0000634c;
extern char DAT_00010ab8[];
extern int DAT_0000f8ec;
extern int DAT_0000f8f0;
extern int DAT_0010a18;
extern int DAT_0010a1c;
extern int DAT_0010a80;
extern int DAT_0010a94;
extern int DAT_0010a9c;

extern int car_setup(void *record, int index, int load);
extern void *member_list_load(char *name, int *array);
extern void scene_setup(int value);

int game_mode_100_start(int value)
{
    int index;
    int result;

    DAT_0011ce44 = 0;
    DAT_0011ce48 = 0;
    DAT_0011ce40 = 0;
    DAT_0011ce3c = 0;
    DAT_0011d220 = 0;
    DAT_0011d21c = 0;
    DAT_0011ce4c = 0xc3a00000u;
    DAT_0011ce50 = 0;
    DAT_0011d224 = 0x3f800000u;
    DAT_0000f8f0 = 0;
    for (index = 0; index < 0x14; index++) {
        DAT_0011ce58[index] = 0x15e;
        DAT_0011cea8[index] = 0x64;
    }
    DAT_0000634c = 0;
    while (DAT_0000634c < DAT_0000f8ec) {
        result = car_setup((void *)(DAT_00010ab8 + DAT_0000634c * 0x590),
                           DAT_0000634c, 1);
        if (result != 0) {
            return result;
        }
        DAT_0000634c = DAT_0000634c + 1;
    }
    DAT_0011d218 = (int)member_list_load((char *)0x47b,
                                         (int *)DAT_0011cef8);
    DAT_0010a9c = 1;
    DAT_00001e04 = 1;
    DAT_0010a80 = -1;
    DAT_0010a18 = 0;
    DAT_00118e1c.bits = DAT_00118e1c.bits | 0x2001;
    DAT_0011d228 = DAT_000007fd;
    DAT_0010a1c = -1;
    DAT_0010a94 = 0;
    DAT_00118e1c.bytes[1] = DAT_00118e1c.bytes[1] & 0xfd;
    scene_setup(value);
    return 0;
}
