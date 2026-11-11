/* The rebuilt `map_file_load` module.
 *
 * The original sits at linear 0x4c94c in STARTH.EXE and holds 27 instructions
 * and 61 bytes up to the `ret` at 0x4c988.  The next function starts at
 * 0x4c98c, so the store record keeps the 64-byte extent and sets no separate
 * function size.  The identity check uses 27 instructions.
 *
 * The only direct caller is `race_data_load` at 0x131a8 with the call site at
 * 0x13347.  It passes the level name `DAT_0011d294` in `eax`, which `race_start`
 * builds as `MAP%d`.  The caller runs `material_load` at 0x4cd9c with the same
 * level name first, so the record takes the section Resource loader.
 *
 * The function copies the level name to the global name buffer `DAT_002154c`
 * through the inline two-byte copy loop.  It then calls `unr_file_load` at
 * 0x4c98c and `mmp_file_load` at 0x4cae4, which build the `%s.UNR` and
 * `%s.MMP` member names from that buffer.  The caller, the two callees, and the
 * shared name buffer support the name `map_file_load`.  The original has no
 * symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The `-oi`
 * flag expands the `strcpy` call into the same two-byte copy loop as the
 * original.  The function uses no floating-point instruction, so the module
 * needs no FPU stub and no assembly implementation.
 */
#include <string.h>

extern char DAT_002154c[];
extern void unr_file_load(void);
extern void mmp_file_load(void);

void map_file_load(char *name)
{
    strcpy(DAT_002154c, name);
    unr_file_load();
    mmp_file_load();
}
