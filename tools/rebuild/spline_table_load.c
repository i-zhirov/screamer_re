/* The rebuilt `spline_table_load` module.
 *
 * The original sits at linear 0x14fb8 in STARTH.EXE and holds 80 instructions
 * and 316 bytes up to the `ret` at 0x150f3.  The next function starts at
 * 0x150f4, so the store record keeps the 316-byte extent and sets no separate
 * function size.  The identity check uses 80 instructions.
 *
 * The only direct caller at 0x133af is `race_data_load` at 0x131a8.  The
 * caller passes the local level name at `[ebp + 0x17e]` in eax, and it then
 * calls the track resource loader at 0x150f4 with the same buffer.  The record
 * takes the name `spline_table_load` with the confidence inferred.
 *
 * The function builds five member names from the level prefix at
 * `DAT_0011d294` with the `sprintf` at 0x4d32c.  The suffixes are `opp`,
 * `anm`, `cam`, and `rpl`.  The fifth suffix is `CNG` for the game mode 0x20,
 * `SLA` for 0x40, and `OST` for every other mode.  The names sit at
 * `DAT_0010f8d8`, `DAT_0010f168`, `DAT_0010f8f4`, `DAT_0010f900`, and
 * `DAT_0010d6e8`.
 *
 * The function then calls `spline_load` at 0x1af70 five times with the level
 * name in eax and the member name in edx.  The third argument receives the
 * spline table pointer.  The function stores the returned count at
 * `DAT_0010f8e8`, `DAT_0010f178`, `DAT_0010f914`, `DAT_0010f918`, and
 * `DAT_0010d6fc`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero, so the
 * harness build defines `SPLINE_TABLE_LOAD_HARNESS` and the function builds the
 * format text in local buffers.  The default build keeps the literals of the
 * original for the later period build.
 */
extern char DAT_0011d294[];
extern int DAT_0011d260;

extern char DAT_0010f8d8[];
extern void *DAT_0010f8e4;
extern int DAT_0010f8e8;

extern char DAT_0010f168[];
extern void *DAT_0010f174;
extern int DAT_0010f178;

extern char DAT_0010f8f4[];
extern void *DAT_0010f90c;
extern int DAT_0010f914;

extern char DAT_0010f900[];
extern void *DAT_0010f910;
extern int DAT_0010f918;

extern char DAT_0010d6e8[];
extern void *DAT_0010d6f4;
extern int DAT_0010d6fc;

extern int sprintf(char *buffer, const char *format, ...);
extern int spline_load(char *archive, char *name, void **table);

#ifdef SPLINE_TABLE_LOAD_HARNESS
static void fill(char *text, char a, char b, char c, char d,
                 char e, char f, char g, char h)
{
    text[0] = a;
    text[1] = b;
    text[2] = c;
    text[3] = d;
    text[4] = e;
    text[5] = f;
    text[6] = g;
    text[7] = h;
    text[8] = 0;
}
#endif

void spline_table_load(char *level)
{
#ifdef SPLINE_TABLE_LOAD_HARNESS
    char opp_format[0x10];
    char anm_format[0x10];
    char cam_format[0x10];
    char rpl_format[0x10];
    char cng_format[0x10];
    char sla_format[0x10];
    char ost_format[0x10];
#else
    char *opp_format = "%sopp";
    char *anm_format = "%sanm";
    char *cam_format = "%scam";
    char *rpl_format = "%srpl";
    char *cng_format = "%sCNG";
    char *sla_format = "%sSLA";
    char *ost_format = "%sOST";
#endif

#ifdef SPLINE_TABLE_LOAD_HARNESS
    fill(opp_format, '%', 's', 'o', 'p', 'p', 0, 0, 0);
    fill(anm_format, '%', 's', 'a', 'n', 'm', 0, 0, 0);
    fill(cam_format, '%', 's', 'c', 'a', 'm', 0, 0, 0);
    fill(rpl_format, '%', 's', 'r', 'p', 'l', 0, 0, 0);
    fill(cng_format, '%', 's', 'C', 'N', 'G', 0, 0, 0);
    fill(sla_format, '%', 's', 'S', 'L', 'A', 0, 0, 0);
    fill(ost_format, '%', 's', 'O', 'S', 'T', 0, 0, 0);
#endif

    sprintf(DAT_0010f8d8, opp_format, DAT_0011d294);
    sprintf(DAT_0010f168, anm_format, DAT_0011d294);
    sprintf(DAT_0010f8f4, cam_format, DAT_0011d294);
    sprintf(DAT_0010f900, rpl_format, DAT_0011d294);
    if (DAT_0011d260 == 0x20) {
        sprintf(DAT_0010d6e8, cng_format, DAT_0011d294);
    } else if (DAT_0011d260 == 0x40) {
        sprintf(DAT_0010d6e8, sla_format, DAT_0011d294);
    } else {
        sprintf(DAT_0010d6e8, ost_format, DAT_0011d294);
    }
    DAT_0010f8e8 = spline_load(level, DAT_0010f8d8, &DAT_0010f8e4);
    DAT_0010f178 = spline_load(level, DAT_0010f168, &DAT_0010f174);
    DAT_0010f914 = spline_load(level, DAT_0010f8f4, &DAT_0010f90c);
    DAT_0010f918 = spline_load(level, DAT_0010f900, &DAT_0010f910);
    DAT_0010d6fc = spline_load(level, DAT_0010d6e8, &DAT_0010d6f4);
}
