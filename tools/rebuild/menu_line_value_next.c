/* The rebuilt `menu_line_value_next` module.
 *
 * The original sits at linear 0x298b4 in STARTH.EXE and holds 30 instructions
 * and 100 bytes up to the `ret` at 0x29917.  The next function starts at
 * 0x29918, so the store record keeps the 100-byte extent and sets no separate
 * function size.  The identity check uses 30 instructions.
 *
 * The only direct caller is at 0x28ec6.  The call is case 0x4d2 of
 * `net_race_screen` at 0x28ba4, and the case calls this function and then
 * `table_lookup` at 0x1f718 with 2.
 *
 * A line record of the table `DAT_00102de8[index]` holds the first string
 * offset at `+0x00`, the item id at `+0x04`, the value index at `+0x08`, and
 * the value count at `+0x0c`.  The menu index is `DAT_0011fb60` and the
 * selected line is `DAT_001fb08[index]`.
 *
 * The function first calls `map_object_progress_sum` at 0x2b3e4.  It returns
 * at once when the sum is not zero.  Otherwise it increments the value index of
 * the selected line and resets the index to zero when it exceeds the value
 * count.  It then sets the overlay word `DAT_0011fac0` to 1.
 *
 * `menu_text_update` at 0x29918 reads the value index as the second string
 * offset and the value count as the highest value, so the increment selects the
 * next value of the selected line.  The progress sum measures the remaining
 * menu animation, so the function waits for the animation to finish.  The name
 * `menu_line_value_next` is inferred and the section is race_frame callees.
 * The original has no symbol.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses the x87 stack only for the zero compare, so the module links
 * the shared `fpu_stub.asm`.  The module uses no assembly implementation.
 */
extern int DAT_0011fb60;
extern int DAT_001fb08[];
extern char *DAT_002de8[];
extern int DAT_0011fac0;

extern float map_object_progress_sum(void);

struct menu_line {
    int string_offset;          /* +0x00 */
    int item_id;                /* +0x04 */
    int value_index;            /* +0x08 */
    int value_count;            /* +0x0c */
};

void menu_line_value_next(void)
{
    struct menu_line *lines;
    struct menu_line *line;
    int index;
    int row;

    if (map_object_progress_sum() != 0.0f) {
        return;
    }
    index = DAT_0011fb60;
    row = DAT_001fb08[index];
    lines = (struct menu_line *)DAT_002de8[index];
    line = &lines[row];
    line->value_index = line->value_index + 1;
    if (line->value_index > line->value_count) {
        line->value_index = 0;
    }
    DAT_0011fac0 = 1;
}
