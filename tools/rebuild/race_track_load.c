/* The rebuilt `race_track_load` module.
 *
 * The original starts at linear 0x13540 in STARTH.EXE and holds 13
 * instructions and 33 bytes up to the `ret` at 0x13560.  The branch at 0x13558
 * jumps over the first `ret` to 0x13561, so the function continues through the
 * `ret` at 0x1357e.  The store record keeps the 64-byte extent to the next
 * function at 0x13580 and sets no separate function size.  The identity check
 * uses 13 instructions.
 *
 * The only direct caller at 0x1294e is `race_session_start` at 0x128e0.  It
 * passes the level value and drops a non-zero result, so the record takes the
 * section `race_session_start callees`.
 *
 * The function runs `opponent_route_setup` at 0x1517c with the level value.
 * A non-zero result returns at once.  It then runs `opponent_records_init` at
 * 0x15378 with the same value.  A non-zero result returns at once.  It returns
 * 0 when both calls succeed.
 *
 * The two opponent calls and the level value support the name
 * `race_track_load`.  The original has no symbol, so the record keeps the name
 * with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and calls two helpers, so the
 * module needs no FPU stub and no assembly implementation.
 */
extern int opponent_route_setup(int load);
extern int opponent_records_init(int load);

int race_track_load(int load)
{
    int result;

    result = opponent_route_setup(load);
    if (result != 0) {
        return result;
    }
    result = opponent_records_init(load);
    if (result != 0) {
        return result;
    }
    return 0;
}
