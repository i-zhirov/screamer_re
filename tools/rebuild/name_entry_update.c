/* The rebuilt `name_entry_update` module.
 *
 * The original sits at linear 0x2ce84 in STARTH.EXE and holds 61 instructions
 * and 225 bytes up to the `ret` at 0x2cf64.  The next function starts at
 * 0x2cf68, so the store record keeps the 228-byte extent and sets no function
 * size.  The identity check uses 61 instructions.
 *
 * The only direct caller at 0x295e7 is case 4 of the race state update at
 * 0x29564.  That update is a `race_frame` callee at 0x2807c, so the record
 * takes the section `race_frame callees`.  The matching state 4 init at
 * 0x2c430 is the `name_entry_screen` module: it builds the 26 A to Z records
 * at `DAT_001db78` and the input at 0x2893a selects the letter.  The update
 * marks the selected record, runs the letter update on each of the 0x50
 * records, and recomputes the menu marker from the current letter index, so
 * the module names the function `name_entry_update`.  The name is inferred,
 * because the original holds no symbol for the function.
 *
 * The call at 0x2b308 selects the record whose key `+0x04` equals the letter
 * code.  The key is the current index `DAT_0011d23c` plus 0x41, the uppercase
 * A.  The call preserves `edx`, so the compiler keeps the loop counter in
 * `edx` across the call.  The module names the callee
 * `map_object_line_select`, the same name the `menu_text_update` module uses.
 *
 * The loop runs the `name_entry_letter_update` module at 0x2b660 on each of
 * the 0x50 records at `DAT_001db78`, stride 0x64.
 *
 * A non-zero letter index places the menu marker from the cosine and the sine
 * of `index * -0.24166098` at the scale 4500.0.  The sine gives
 * `DAT_001203a8`, the cosine gives `DAT_001203b0`, and the heading
 * `index * 0.24166098` gives `DAT_00120394`.  A zero index clears the two
 * integer cells and stores the default 0x1194 at `DAT_001203b0`.
 *
 * The original calls the cosine helper at 0x4da66, the sine helper at 0x4da72,
 * and the truncate helper at 0x4d9f4.  This harness links no math library, so
 * the local FPU stub supplies `cos` and `sin` from the hardware and makes
 * `__CHP` truncate toward zero.  The constants arrive through globals, because
 * the minimal image does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_0011d23c;
extern char DAT_001db78[];
extern int DAT_001203a8;
extern int DAT_001203b0;
extern float DAT_00120394;

extern float name_entry_cos_sine_scale;
extern float name_entry_marker_scale;
extern float name_entry_heading_scale;

extern void map_object_line_select(int key);
extern void name_entry_letter_update(char *record);
extern double sin(double value);
extern double cos(double value);

void name_entry_update(void)
{
    float angle;
    float cos_value;
    float sin_value;
    int index;
    int i;

    map_object_line_select(DAT_0011d23c + 0x41);
    for (i = 0; i < 0x50; i++) {
        name_entry_letter_update(DAT_001db78 + i * 0x64);
    }

    index = DAT_0011d23c;
    if (index != 0) {
        angle = (float)index * name_entry_cos_sine_scale;
        cos_value = (float)cos((double)angle);
        sin_value = (float)sin((double)angle);
        DAT_001203a8 = (int)(sin_value * name_entry_marker_scale);
        DAT_001203b0 = (int)(cos_value * name_entry_marker_scale);
        DAT_00120394 = (float)index * name_entry_heading_scale;
    } else {
        DAT_001203a8 = 0;
        DAT_00120394 = 0.0f;
        DAT_001203b0 = 0x1194;
    }
}
