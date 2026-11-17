/* The rebuilt `cd_check` module.
 *
 * The original sits at linear 0x157b8 in STARTH.EXE and holds 57 instructions
 * and 185 bytes up to the `ret` at 0x15870.  The next candidate starts at
 * 0x15874, so the store record keeps the 188-byte extent and sets no separate
 * function size.  The identity check uses 57 instructions.
 *
 * The only direct caller is `cd_check_screen` at 0x15874, which calls this
 * function at 0x1587a and at 0x15955.  It returns at once when this function
 * returns 0, so the insert-CD screen runs only when the CD is missing or
 * wrong.  The module takes the section `race_loop callees`, like its caller.
 *
 * The function gets the real-mode CD driver response through `sub_26014`.  A
 * zero response returns 1.  Otherwise the function builds four field pointers
 * at offsets 8, 0x28, 0x13e, and 0x23e of the response.  It compares each field
 * with the text `PC`, `SCREAMER`, `VIE`, and `SCREAMER` through the
 * case-insensitive length-limited compare `strnicmp`.  A failed compare stops
 * the chain and returns 1.  A full match returns 0.
 *
 * The four texts live at object-3 offsets 0x67e, 0x681, 0x68a, and 0x68e.  The
 * harness holds them in the cells `DAT_0010067e`, `DAT_00100681`,
 * `DAT_0010068a`, and `DAT_0010068e`, because the constant segment of the
 * minimal image is not loaded.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly.
 */
extern char DAT_0010067e[];
extern char DAT_00100681[];
extern char DAT_0010068a[];
extern char DAT_0010068e[];

extern void *sub_26014(void);
extern int strnicmp(const char *left, const char *right, int count);

int cd_check(void)
{
    int result;
    char *response;
    char *pc;
    char *screamer;
    char *vie;
    char *screamer_b;

    result = 0;
    response = sub_26014();
    if (response == 0)
        return 1;
    pc = response + 8;
    screamer = response + 0x28;
    vie = response + 0x13e;
    screamer_b = response + 0x23e;
    if (strnicmp(pc, DAT_0010067e, 2) != 0) {
        result = 1;
    } else if (strnicmp(screamer, DAT_00100681, 8) != 0) {
        result = 1;
    } else if (strnicmp(vie, DAT_0010068a, 3) != 0) {
        result = 1;
    } else if (strnicmp(screamer_b, DAT_0010068e, 8) != 0) {
        result = 1;
    }
    return result;
}
