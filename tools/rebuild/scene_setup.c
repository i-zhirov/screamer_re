/* The rebuilt `scene_setup` module.
 *
 * The original sits at linear 0x15984 in STARTH.EXE and holds 70 instructions
 * and 240 bytes up to the `ret` at 0x15a73.  The next record starts at 0x15a80
 * after the alignment padding, so the store record keeps the 252-byte extent
 * and sets no separate function size.  The identity check uses 70 instructions.
 *
 * The six direct callers are the game mode starts at 0x12c0c, 0x12d0c,
 * 0x12e18, 0x12f50, 0x130a0, and 0x13120.  race_session_start at 0x128e0
 * dispatches the game mode to them, and every caller passes the mode argument.
 * The caller already declares the call as `scene_setup`, so the record keeps
 * that name with the confidence inferred.
 *
 * The function builds three names from the level prefix at DAT_0011d294:
 *
 *     %s%s    the level prefix and the suffix i for the flag bit 8 of
 *             DAT_0011d274, or no suffix
 *     %s%c    the level prefix and the letter 0x41 plus DAT_0011d2b4
 *     %sdat   the level prefix and dat
 *
 * A non-zero mode loads the base SOUND_FX list SFXSBASE and the level SOUND_FX
 * list through list_file_load at 0x15460 into the table at DAT_00115224.  The
 * first call fills the table from the start, and the second call fills it after
 * the first count times the 0x24-byte stride.  The two counts sum at
 * DAT_0018d84, and the first count stays at DAT_0018d88.  The function then
 * loads the player sound lists through the helper at 0x24600 and loads the
 * sample bank through sound_bank_load at 0x1f2e0.
 *
 * The original has no floating-point instruction.  It has the EBP frame, so
 * the module builds with `-of+`.
 */
extern char DAT_0011d294[];
extern int DAT_0011d274;
extern int DAT_0011d2b4;
extern int DAT_00115224[];
extern int DAT_0018d88;
extern int DAT_0018d84;

extern int sprintf(char *buffer, const char *format, ...);
extern int list_file_load(char *type, char *name, char *list, void *table,
                          int stride, int count);
extern void func_24600(void);
extern void sound_bank_load(void);

void scene_setup(int mode)
{
    char base[0x28];
    char letter[0x80];
    char list[0x100];

    sprintf(base, "%s%s", DAT_0011d294, (DAT_0011d274 & 8) != 0 ? "i" : "");
    sprintf(letter, "%s%c", DAT_0011d294, DAT_0011d2b4 + 0x41);
    sprintf(list, "%sdat", DAT_0011d294);
    if (mode != 0) {
        DAT_0018d88 = list_file_load(
            "GEN", "SOUND_FX", "SFXSBASE", DAT_00115224, 0x24, 0x24);
        DAT_0018d84 = DAT_0018d88 + list_file_load(
            base, "SOUND_FX", list,
            (char *)DAT_00115224 + DAT_0018d88 * 0x24, 0x24, 0x24);
        func_24600();
        sound_bank_load();
    }
}
