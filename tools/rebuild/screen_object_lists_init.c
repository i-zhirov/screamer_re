/* The rebuilt `screen_object_lists_init` module.
 *
 * The original sits at linear 0x2dff4 in STARTH.EXE and holds 67 instructions
 * and 191 bytes up to the `ret` at 0x2e0b2.  The next function starts at
 * 0x2e0b4, so the store record keeps the 192-byte extent and sets no separate
 * function size.  The identity check uses 67 instructions.
 *
 * The eight direct callers are `menu_frame` at 0x29f80 (call at 0x2a19b),
 * `menu_objects_init` at 0x2aab8 (call at 0x2b037), `car_select_screen` at
 * 0x2bedc (call at 0x2bff3), `track_select_screen` at 0x2c188 (call at
 * 0x2c295), `name_entry_screen` at 0x2c430 (call at 0x2c5e8),
 * `championship_screen_init` at 0x2c7a8 (call at 0x2c7e5),
 * `race_records_screen_init` at 0x2c954 (call at 0x2c9a2), and
 * `net_race_lobby_init` at 0x2ca40 (call at 0x2ca6f).  Every caller runs the
 * function while it prepares a menu or a race screen, and the function resets
 * and rebinds the three object record lists that the screen draws.  The
 * callers sit in the `frame_update callees` and the `race_frame callees`
 * sections, so the record takes the section `Program functions`.
 *
 * The function walks three tables in object 3:
 *
 *   - The 0x50 map object records at object-3 offset 0x1db78, stride 0x64.
 *     The helper `map_object_record_init` at 0x2b450 stores the record index
 *     in the first field, sets the object field +0x08 to -1, and binds the
 *     object slot.  The module `menu_text_update` at 0x29918 and the module
 *     `map_object_place` at 0x2b044 read and fill the same records.
 *   - The 0xe car display records at object-3 offset 0x1d550, stride 0x44.
 *     The module `car_select_screen` at 0x2bedc and the module
 *     `track_select_screen` at 0x2c188 fill the values after this call.
 *   - The 0xc track display records at object-3 offset 0x1d908, stride 0x34.
 *
 * The last two loops call the helper `object_list_bind` at 0x11850 with the
 * object-list table 0x6530, the list state 0x6350, and the record.  The helper
 * reads the record index and binds the record into the display list.
 *
 * The name `screen_object_lists_init` is inferred, because the original holds
 * no symbol for the function.  The original has the EBP frame, so the module
 * builds with `-of+`.  The function holds no floating-point instruction, so it
 * needs no FPU stub, and the module uses no assembly.
 */
extern char DAT_0011db78[];
extern char DAT_0011d550[];
extern char DAT_0011d908[];

extern void map_object_record_init(char *record, int index);
extern void object_list_bind(void *table, char *name, void *record);

void screen_object_lists_init(void)
{
    int index;

    for (index = 0; index < 0x50; index++) {
        map_object_record_init(DAT_0011db78 + index * 0x64, index);
    }
    for (index = 0; index < 0xe; index++) {
        object_list_bind((void *)0x6530, (char *)0x6350,
                         DAT_0011d550 + index * 0x44);
    }
    for (index = 0; index < 0xc; index++) {
        object_list_bind((void *)0x6530, (char *)0x6350,
                         DAT_0011d908 + index * 0x34);
    }
}
