/* The rebuilt `race_start` module.
 *
 * The original sits at linear 0x27770 in STARTH.EXE and holds 66 instructions
 * and 251 bytes: the five register saves, the frame, the 4-byte local slot,
 * the four screen-rectangle stores, the `choice_load` call at `0x1bbc8`, the
 * five state copies, the context reads, the `state_cells_set` call at
 * `0x8bd54`, the two clears, the `MAP%d` build through `sprintf` at `0x4d32c`,
 * the three mode stores, the `race_session_start` call at `0x128e0`, and the
 * result test.
 *
 * The function starts one level.  It sets the screen rectangle to 640 by 400,
 * loads the choice file, copies five state globals, sets the level to 1, and
 * reads the race context.  It copies the word at context `+0x48`, stores the
 * index at context `+8`, and passes the table value `DAT_001021d8[index]` to
 * the helper at `0x8bd54`.  It clears two state words, builds `MAP%d` from the
 * counter `DAT_001207f8 + 1` into `DAT_0011d294`, sets the game mode to 0x100,
 * copies the old `DAT_0011d274` to `DAT_0011d22c`, clears `DAT_0011d274`, and
 * calls `race_session_start(1)`.  A non-zero result is returned at once.
 *
 * The call target `0x1bbc8` builds `%s\config\choice.dat` and reads the file,
 * so the module names it `choice_load`.  The call target `0x8bd54` stores its
 * argument in eight data cells and sets two derived cells, so the module names
 * it `state_cells_set`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_01015ce4;
extern int DAT_01015ce8;
extern int DAT_01015cec;
extern int DAT_01015cf0;
extern int DAT_001207f4;
extern int DAT_0012080c;
extern int DAT_00120808;
extern int DAT_00120810;
extern int DAT_00120814;
extern int DAT_0011d2a8;
extern int DAT_0011d2c0;
extern int DAT_0011d2bc;
extern int DAT_0011d2c4;
extern int DAT_0011d2a4;
extern int DAT_0011d2b4;
extern int DAT_0011d2b8;
extern int DAT_0011d2cc;
extern int DAT_00118e30;
extern int DAT_0011ce54;
extern int DAT_001207f8;
extern char DAT_0011d294[];
extern int DAT_0011d260;
extern int DAT_0011d22c;
extern int DAT_0011d274;
extern int DAT_001021d8[];
extern void *DAT_00002dec;
extern void choice_load(void);
extern void state_cells_set(int value);
extern int sprintf(char *buffer, const char *format, ...);
extern int race_session_start(int value);

int race_start(void)
{
    int result;
    int index;
    void *context;

    DAT_01015ce4 = 0;
    DAT_01015ce8 = 0x280;
    DAT_01015cec = 0;
    DAT_01015cf0 = 0x190;
    choice_load();
    DAT_0011d2a8 = DAT_001207f4;
    DAT_0011d2c0 = DAT_0012080c;
    DAT_0011d2bc = DAT_00120808;
    DAT_0011d2c4 = DAT_00120810;
    DAT_0011d2a4 = DAT_00120814;
    DAT_0011d2b4 = 1;
    context = DAT_00002dec;
    DAT_0011d2cc = *(int *)((char *)context + 0x48);
    index = *(int *)((char *)context + 8);
    DAT_0011d2b8 = index;
    state_cells_set(DAT_001021d8[index]);
    DAT_00118e30 = 0;
    DAT_0011ce54 = 0;
    sprintf(DAT_0011d294, "MAP%d", DAT_001207f8 + 1);
    DAT_0011d260 = 0x100;
    DAT_0011d2b4 = 1;
    DAT_0011d22c = DAT_0011d274;
    DAT_0011d274 = 0;
    result = race_session_start(1);
    if (result != 0) {
        return result;
    }
    return 0;
}
