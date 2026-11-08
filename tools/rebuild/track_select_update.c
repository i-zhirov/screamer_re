/* The rebuilt `track_select_update` module.
 *
 * The original sits at linear 0x2cc98 in STARTH.EXE and holds 124 instructions
 * and 491 bytes up to the `ret` at 0x2ce82.  The next function starts at
 * 0x2ce84, so the store record keeps the 492-byte extent and sets no function
 * size.  The identity check uses 124 instructions.
 *
 * The only direct caller at 0x295dd is case 3 of the race state update at
 * 0x29564.  That update is a `race_frame` callee at 0x2807c, so the record
 * takes the section `race_frame callees`.  The matching state 3 init at
 * 0x2c188 builds the 12 map display records at `DAT_0011d908`, and its module
 * is `track_select_screen`.  The update walks the same records and writes the
 * three object angles, so the module names the function
 * `track_select_update`.  The name is inferred, because the original holds no
 * symbol for the function.
 *
 * The event code 0x8844 and the selection cells `DAT_001203c8` and
 * `DAT_00120b5c` are the network track selection handshake of
 * `net_race_lobby`; the mode `DAT_0011d260` is 0x10 in the network race.
 *
 * The function adds `DAT_0018e08 * pi/4` to the heading at `+0x0c` of each
 * record.  It resets the heading to 0 at or above 2*pi.  It then converts the
 * three record angles at `+0x08`, `+0x0c`, and `+0x10` to the object angles at
 * `+0x14`, `+0x18`, and `+0x1c` with the factor 1024.0 / pi and the mask
 * 0x7ff.  The object pointer comes from the table `DAT_0010653c` with the
 * record object index at `+0x00`.
 *
 * The original calls the truncate helper at 0x4d9f4 before each `fistp`.  This
 * harness links no math library, so the local `track_select_update_fpu.asm`
 * makes `__CHP` truncate toward zero.  The shared `fpu_stub.asm` defines
 * `__CHP` as a bare return, which leaves the round-to-nearest mode.  The float
 * constants arrive through globals, because the minimal image does not load
 * the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_0011d260;
extern int DAT_001203c8;
extern int DAT_00120b5c;
extern float DAT_0018e08;
extern float DAT_001011e3;
extern float DAT_001011e7;
extern float DAT_001011eb;
extern float DAT_001011ef;

struct track_record {
    int object_index;
    int field_04;
    float angle_x;
    float angle_y;
    float angle_z;
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

extern int net_player_add(int id);

void track_select_update(void)
{
    struct track_record *record;
    struct display_object *object;
    int index;
    int saved;

    if (DAT_0011d260 == 0x10) {
        net_player_add(3);
        saved = DAT_001203c8;
        DAT_001203c8 = DAT_00120b5c;
        net_player_add(0x8844);
        DAT_001203c8 = saved;
    }
    for (index = 0; index < 0xc; index++) {
        record = &DAT_0011d908[index];
        record->angle_y = record->angle_y + DAT_0018e08 * DAT_001011e3;
        if (record->angle_y >= DAT_001011e7) {
            record->angle_y = 0.0f;
        }
        object = DAT_0010653c[record->object_index][0];
        object->angle_x =
            (int)(record->angle_x * DAT_001011eb / DAT_001011ef) & 0x7ff;
        object->angle_y =
            (int)(record->angle_y * DAT_001011eb / DAT_001011ef) & 0x7ff;
        object->angle_z =
            (int)(record->angle_z * DAT_001011eb / DAT_001011ef) & 0x7ff;
    }
}
