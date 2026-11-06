/* The rebuilt `table_lookup` module.
 *
 * The original sits at linear 0x1f718 in STARTH.EXE and holds 28 instructions
 * and 74 bytes: the register saves, the frame, the argument slot, the guard on
 * `DAT_00008058`, the index scaling, the table base, and the lookup call at
 * `0xc9db5`.
 *
 * The function returns the table entry for the index when the table flag is
 * set, and `-1` otherwise.
 */
extern unsigned int DAT_00008058;
extern unsigned int DAT_00018d94;
extern int table_read(unsigned int base, int index, unsigned int entry);

int table_lookup(int index)
{
    if (DAT_00008058 != 0) {
        return table_read(DAT_00018d94, index, 0x16034 + index * 116);
    }
    return -1;
}
