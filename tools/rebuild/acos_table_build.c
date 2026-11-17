/* The rebuilt `acos_table_build` module.
 *
 * The original sits at linear 0x4d260 in STARTH.EXE and holds 40 instructions
 * and 121 bytes up to the `ret` at 0x4d2d8.  Three pad bytes follow the `ret`,
 * and the next record starts at 0x4d2dc, so the store record keeps the
 * 124-byte extent and sets no separate function size.  The identity check uses
 * 40 instructions.
 *
 * The loop walks the value from -1.0 to 1.0 with the step 1/256.  For each
 * value it forms the inverse cosine through the Watcom helper at 0xc5e42,
 * scales it by 40.42535442040344, rounds the product to a float, and truncates
 * it to an integer.  The value times 256 selects the slot of the table through
 * the pointer at 0xc55f4, so the table holds 513 entries from index -256 to
 * index 256.  The table maps a cosine in the range -1 to 1 to an angle in the
 * range 0 to 127.
 *
 * The only direct caller is `stream_layer_init` at 0x4c810, with the call site
 * at 0x4c834.  That function first builds the reciprocal table at 0x4d214 and
 * the row offset table at 0x4d2dc, then builds this table, then loads the
 * perspective table and the visibility table.  The caller sits in the section
 * The TABS table loaders, so the record takes the same section.  The caller,
 * the table, and the inverse cosine support the name `acos_table_build`.  The
 * original has no symbol, so the record keeps the name with the confidence
 * inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * original truncates through the Watcom helper at 0x4d9f4, so the local FPU
 * stub makes `__CHP` truncate toward zero.  The original calls the inverse
 * cosine helper through the hardware, so the local FPU stub supplies `acos`
 * from the hardware.  The harness does not load the constant segment, so the
 * scale and the index scale arrive through globals that the harness sets from
 * their bit patterns.  The module uses no assembly implementation.
 */
extern double acos(double value);
extern double acos_scale;
extern double acos_index_scale;
extern int *DAT_000c55f4;

void acos_table_build(void)
{
    float step;
    float value;
    volatile float angle;
    int scaled;
    int index;

    step = 0.00390625f;
    value = -1.0f;
    while (value <= 1.0f) {
        angle = (float)(acos((double)value) * acos_scale);
        scaled = (int)angle;
        index = (int)((double)value * acos_index_scale);
        DAT_000c55f4[index] = scaled;
        value = value + step;
    }
}
