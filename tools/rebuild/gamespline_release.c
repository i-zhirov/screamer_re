/* The rebuilt `gamespline_release` module.
 *
 * The original sits at linear 0x1b180 in STARTH.EXE and holds 9 instructions
 * and 22 bytes up to the `ret` at 0x1b195.  The next record `track_marker_load`
 * starts at 0x1b198 after the two padding bytes at 0x1b196 and 0x1b197, so the
 * store record keeps the 24-byte extent and sets no separate function size.
 * The identity check uses 9 instructions.
 *
 * The direct callers are `race_data_release` at 0x156a8 with the call site at
 * 0x1570a and `race_end` at 0x279a4 with the call site at 0x279cc.  Both
 * callers run the function in a teardown.
 *
 * The function frees the `GAMESPLINE` resource at object-3 offset 0x967 from
 * the archive `DAT_000062ac` through `resource_free_name` at 0x2f326.  The
 * string at linear 0x100967 holds `GAMESPLINE`.  The function loads the name
 * pointer in edx and the archive context in eax before the call.
 *
 * The sibling `spline_load` at 0x1af70 gets the same `GAMESPLINE` resource at
 * object-3 offset 0x95c, so the two functions form the game spline data load
 * and release pair.  The loader sits in the section Resource loader, so this
 * module takes the same section.  The distinct `SPLINE` resource at linear
 * 0x100960 belongs to another loader, so the name keeps the full `game`
 * prefix.
 *
 * The `GAMESPLINE` resource, the caller group, and the sibling loader support
 * the name `gamespline_release`.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly body.
 */
extern int DAT_000062ac;

extern void resource_free_name(int context, char *name);

void gamespline_release(void)
{
    resource_free_name(DAT_000062ac, "GAMESPLINE");
}
