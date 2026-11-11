/* The rebuilt `track_resource_load` module.
 *
 * The original sits at linear 0x150f4 in STARTH.EXE and holds 29 instructions
 * and 92 bytes up to the `ret` at 0x1514f.  The next record starts at 0x1517c,
 * so the record keeps the 92-byte extent and sets no separate function size.
 * The identity check uses 29 instructions.
 *
 * The only direct caller at 0x133ba is `race_data_load` at 0x131a8.  The caller
 * puts the level name at [ebp + 0x17e] in eax, so the record takes the section
 * race_session_start callees.
 *
 * The function calls `track_marker_load` at 0x1b198 with the level name, the
 * name at DAT_0001d294, and the track node table at DAT_00007250.  It stores
 * the returned count at DAT_00007ed0.
 *
 * When bit 0x40 of the game mode byte at DAT_0001d260 is set, the function
 * builds the name `%ssla` with `sprintf` at 0x4d32c from the name at
 * DAT_0001d294, calls `track_marker_load` again with the SLA marker table at
 * DAT_00007890, and stores the returned count at DAT_00007ed4.
 *
 * The node table, the marker table, and the SLA mode gate support the name
 * track_resource_load.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * volatile `saved` local keeps the level pointer in a stack slot across the
 * `sprintf` call, as the original does.  The function holds no floating-point
 * instruction, so it needs no FPU stub and the module uses no assembly.
 */
extern char DAT_0001d260;
extern char DAT_0001d294[];
extern char DAT_00007250[];
extern char DAT_00007890[];
extern int DAT_00007ed0;
extern int DAT_00007ed4;
extern int sprintf(char *buffer, const char *format, ...);
extern int track_marker_load(char *archive, char *name, void *table);

void track_resource_load(char *level)
{
    char *volatile saved = level;
    char name[0x68];

    DAT_00007ed0 = track_marker_load(saved, DAT_0001d294, DAT_00007250);
    if ((DAT_0001d260 & 0x40) != 0) {
        sprintf(name, "%ssla", DAT_0001d294);
        DAT_00007ed4 = track_marker_load(saved, name, DAT_00007890);
    }
}
