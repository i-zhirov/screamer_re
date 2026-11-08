/* The rebuilt `track_select_screen` module.
 *
 * The original sits at linear 0x2c188 in STARTH.EXE and holds 164 instructions
 * and 679 bytes up to the `ret` at 0x2c42e.  The store record keeps the
 * 680-byte extent, which ends at the next function at 0x2c430, and sets no
 * function size.  The identity check uses 164 instructions.
 *
 * The only direct caller at 0x2beb7 is case 3 of the state init dispatcher at
 * 0x2be94.  The matching state update is case 3 of `race_state_update` at
 * 0x29564, which calls 0x2cc98.  That update walks the 12 records at
 * `DAT_0011d908`, and `race_panel_3` at 0x2e58c draws the text from the same
 * table with the index `DAT_001203c8`.  The init loads `menu3h.pcx`, the map
 * slots hold `AMAP1` through `AMAP6` twice, and the state 3 selection follows
 * the state 2 car selection, so the module names the function
 * `track_select_screen`.  The name is inferred, because the original holds no
 * symbol for the function.
 *
 * The init sets `DAT_0011d240` at 1, the factors `DAT_00120378` at 1.5 and
 * `DAT_00120380` at 0.95, and `DAT_0011fac8` at 1.  It loads `menu3h.pcx`
 * from `gfx\graf.dat` with the palette at object-3 offset 0xcf5c, presents the
 * buffer, copies 0x3e800 bytes to `DAT_0021528`, fills the screen, and
 * presents it again.
 *
 * The selection index `DAT_001203c8` scales to the offset `-40000 * index` at
 * `DAT_00203a8` and `DAT_002039c`.  The function sets the remaining menu
 * constants and runs the menu reset at 0x2dff4.
 *
 * The loop builds 12 map display records at `DAT_0011d908`, stride 0x34.  Each
 * record holds an object index at `+0x00` and three angles at `+0x08`, `+0x0c`,
 * and `+0x10`.  The function sets the angles 0.0, pi/2, and 0.0, places the
 * object from the table `DAT_0010653c` at `x = -40000 * i`, `y = 0`, and
 * `z = 0xb54`, and converts the three angles to the object angles at `+0x14`,
 * `+0x18`, and `+0x1c` with the factor 1024.0 / pi and the mask 0x7ff.  It then
 * refreshes the object list at 0x6530 with the name 0x6350.
 *
 * The original calls the truncate helper at 0x4d9f4.  This harness links no
 * math library, so the local FPU stub makes `__CHP` truncate toward zero.
 * The shared `fpu_stub.asm` defines `__CHP` as a bare return, which leaves the
 * round-to-nearest mode and rounds a negative value away from zero.  The float
 * constants arrive through globals, because the minimal image does not load
 * the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_0011d240;
extern float DAT_00120378;
extern float DAT_00120380;
extern int DAT_0011fac8;
extern void *DAT_0011fad0;
extern void *DAT_0021528;
extern int DAT_001203c8;
extern int DAT_00203b0;
extern int DAT_00203a4;
extern float DAT_0020384;
extern int DAT_00203a8;
extern int DAT_00203a0;
extern int DAT_002039c;
extern int DAT_00203ac;
extern int DAT_00109a0;
extern float DAT_0020388;
extern float DAT_0020394;
extern int DAT_00109a4;
extern float DAT_002038c;
extern int DAT_00109a8;
extern float DAT_002037c;
extern float DAT_0018e08;
extern int DAT_000007f9;
extern float DAT_0010114c;
extern float DAT_00101150;

struct track_record {
    int object_index;
    int field_04;
    int angle_x;
    int angle_y;
    int angle_z;
    char rest[0x20];
};

struct display_object {
    int field_00;
    int field_04;
    int x;
    int y;
    int z;
    int angle_x;
    int angle_y;
    int angle_z;
};

extern struct track_record DAT_0011d908[];
extern struct display_object *DAT_0010653c[][7];

extern int pcx_load(char *file, char *member, void *pixels, char *palette);
extern void sub_15150(void);
extern void video_present(float scale, int offset);
extern void *memcpy(void *target, const void *source, unsigned int count);
extern void video_fill(void);
extern void sub_2dff4(void);
extern void object_list_refresh(void *table, char *name, int *record);
extern void race_heading_update(void);

void track_select_screen(void)
{
    struct display_object *object;
    struct track_record *record;
    int index;
    int x;
    int y;
    int z;

    DAT_0011d240 = 1;
    DAT_00120378 = 1.5f;
    DAT_00120380 = 0.95f;
    DAT_0011fac8 = 1;

    if (pcx_load((char *)0x113f, (char *)0x1134, DAT_0011fad0,
            (char *)0xcf5c) != 0) {
        sub_15150();
    }
    video_present(0.0f, 0);
    memcpy(DAT_0021528, DAT_0011fad0, 0x3e800);
    video_fill();
    video_present(0.0f, 0);

    DAT_00203ac = 0x12c0;
    DAT_00203a0 = 0x12c0;
    DAT_00203b0 = 0x32c8;
    DAT_00203a4 = 0x32c8;
    DAT_0020388 = 0.0f;
    DAT_0020394 = 0.0f;
    DAT_00109a4 = 0;
    DAT_002038c = 0.0f;
    DAT_00109a8 = 0;
    DAT_002037c = 0.0f;
    DAT_00203a8 = -40000 * DAT_001203c8;
    DAT_002039c = DAT_00203a8;
    DAT_0020384 = -0.62831855f;
    DAT_00109a0 = 0x734;
    sub_2dff4();

    x = 0;
    y = 0;
    z = 0xb54;
    for (index = 0; index < 0xc; ++index) {
        record = &DAT_0011d908[index];
        record->angle_x = 0;
        record->angle_y = 0x3fc90fdb;
        record->angle_z = 0;
        object = DAT_0010653c[record->object_index][0];
        object->x = x;
        object->y = y;
        object->z = z;
        object->angle_x =
            (int)(*(float *)&record->angle_x * DAT_0010114c
                / DAT_00101150) & 0x7ff;
        object->angle_y =
            (int)(*(float *)&record->angle_y * DAT_0010114c
                / DAT_00101150) & 0x7ff;
        object->angle_z =
            (int)(*(float *)&record->angle_z * DAT_0010114c
                / DAT_00101150) & 0x7ff;
        object_list_refresh((void *)0x6530, (char *)0x6350, (int *)record);
        x -= 0x9c40;
    }

    DAT_0018e08 = 0.08f;
    DAT_000007f9 = 8;
    race_heading_update();
}
