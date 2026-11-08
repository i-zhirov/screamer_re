/* The rebuilt `car_select_update` module.
 *
 * The original sits at linear 0x2cb20 in STARTH.EXE and holds 103 instructions
 * and 373 bytes up to the `ret` at 0x2cc94.  The next function starts at
 * 0x2cc98, so the store record keeps the 376-byte extent and sets no separate
 * function size.  The identity check uses 103 instructions.
 *
 * The only direct caller is case 2 of `race_state_update` at 0x29564, with the
 * call site 0x295d3.  `race_state_update` is a `race_frame` callee at 0x2807c,
 * so the section is `race_frame callees`.  The matching init is
 * `car_select_screen` at 0x2bedc, which builds the same 14 records.  The
 * function advances the selected car angle and writes the three object angles,
 * so the module names the function `car_select_update`.  The name is inferred,
 * because the original holds no symbol for the function.
 *
 * The function walks the 14 records at `DAT_0011d550`, stride 0x44.  Each
 * record holds an object index at `+0x00` and three float angles at `+0x08`,
 * `+0x0c`, and `+0x10`.
 *
 * The step is `DAT_0018e08 * pi/2`.  The function adds the step to the record
 * angle `+0x0c`, and it clears the angle to 0 when the sum reaches `2 * pi`.
 * The object from the table `DAT_0010653c` takes the three record angles at
 * the object offsets `+0x14`, `+0x18`, and `+0x1c`, scaled by 1024.0 / pi and
 * masked with 0x7ff.
 *
 * The original calls the truncate helper at 0x4d9f4 before each `fistp`.  This
 * harness links no math library, so the local FPU stub makes `__CHP` truncate
 * toward zero.  The float constants arrive through globals, because the
 * minimal image does not load the constant segment.  The original has the EBP
 * frame, so the module builds with `-of+`.
 */
extern float DAT_0018e08;
extern float DAT_001011d3;
extern float DAT_001011d7;
extern float DAT_001011db;
extern float DAT_001011df;

struct car_record {
    int object_index;
    int field_04;
    int angle_x;
    int angle_y;
    int angle_z;
    char rest[0x30];
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

extern struct car_record DAT_0011d550[];
extern struct display_object *DAT_0010653c[][7];

void car_select_update(void)
{
    struct car_record *record;
    struct display_object *object;
    int index;

    for (index = 0; index < 0xe; ++index) {
        record = &DAT_0011d550[index];
        *(float *)&record->angle_y =
            *(float *)&record->angle_y + DAT_0018e08 * DAT_001011d3;
        if (*(float *)&record->angle_y >= DAT_001011d7) {
            record->angle_y = 0;
        }
        object = DAT_0010653c[record->object_index][0];
        object->angle_x = (int)(*(float *)&record->angle_x
            * DAT_001011db / DAT_001011df) & 0x7ff;
        object->angle_y = (int)(*(float *)&record->angle_y
            * DAT_001011db / DAT_001011df) & 0x7ff;
        object->angle_z = (int)(*(float *)&record->angle_z
            * DAT_001011db / DAT_001011df) & 0x7ff;
    }
}
