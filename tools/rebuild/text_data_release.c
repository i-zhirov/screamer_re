/* The rebuilt `text_data_release` module.
 *
 * The original sits at linear 0x1b58c in STARTH.EXE and holds 9 instructions
 * and 22 bytes up to the `ret` at 0x1b5a1.  The next record `pcx_load` starts
 * at 0x1b5a4 after the two padding bytes at 0x1b5a2 and 0x1b5a3, so the store
 * record keeps the 24-byte extent and sets no separate function size.  The
 * identity check uses 9 instructions.
 *
 * The direct callers are `race_data_release` at 0x156a8 with the call site at
 * 0x156ef, `race_end` at 0x279a4 with the call site at 0x27a08, and
 * `race_cleanup` at 0x2a1b0 with the call site at 0x2a1f0.  Every caller runs
 * the function in a teardown.
 *
 * The function frees the `TEXT` resource at object-3 offset 0x9ea from the
 * archive `DAT_000062ac` through `resource_free_name` at 0x2f326.  The string
 * at linear 0x1009ea holds `TEXT`.  The function loads the name pointer in
 * edx and the archive context in eax before the call.
 *
 * The sibling `member_list_load` at 0x1b394 gets the same `TEXT` resource at
 * object-3 offset 0x9e5, so the two functions form the text data load and
 * release pair.  The loader sits in the section frame_update callees, so this
 * module takes the same section.
 *
 * The `TEXT` resource, the caller group, and the sibling loader support the
 * name `text_data_release`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly body.
 */
extern int DAT_000062ac;

extern void resource_free_name(int context, char *name);

void text_data_release(void)
{
    resource_free_name(DAT_000062ac, "TEXT");
}
