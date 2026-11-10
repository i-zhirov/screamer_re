/* The rebuilt `traffic_reset` module.
 *
 * The original sits at linear 0x1ec7c in STARTH.EXE and holds 44 instructions
 * and 134 bytes up to the `ret` at 0x1ed01.  The next function starts at
 * 0x1ed04, so the store record keeps the 136-byte extent and sets no separate
 * function size.  The identity check uses 44 instructions.
 *
 * `traffic_step` at 0x1eae0 calls this function at 0x1eb27 at the start of the
 * wait and at 0x1eb64 after each timeout.  The function saves the player state
 * through `player_state_snapshot` at 0x1ed04.  It then marks the player traffic
 * record as closed and registers every other traffic record through 0xc7478.
 * The caller uses the function to reset the traffic table, so the record takes
 * the name `traffic_reset` with the confidence inferred.
 *
 * The player object sits at object-3 offset 0xf91c.  The traffic table sits at
 * object-3 offset 0x13cd8.  A record holds 0x2a8 bytes, and its state is at
 * `+0x00`.  The player index is the global at object-3 offset 0x1d250, and the
 * record count is the global at object-3 offset 0x15218.  The shared counter
 * sits at object-3 offset 0x634c.
 *
 * The function stores -1 in the state of the player record.  It then walks the
 * records below the count.  It skips the player index.  For each other record
 * it calls `traffic_register` at 0xc7478 with the record 0x10100, the size
 * 0xc, the name 0x1010c, the value 0x74, and the index.  It repeats the call
 * until the call returns a non-zero value.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU
 * stub and no assembly.
 */
extern int DAT_0011d250;
extern int DAT_00115218;
extern int DAT_0000634c;
extern unsigned char DAT_00113cd8[];

extern void player_state_snapshot(void *object);
extern int traffic_register(void *record, int size, void *name, int value,
    int index);

void traffic_reset(void)
{
    player_state_snapshot((void *)0xf91c);
    *(int *)(DAT_00113cd8 + DAT_0011d250 * 0x2a8) = -1;
    DAT_0000634c = 0;
    while (DAT_0000634c < DAT_00115218) {
        if (DAT_0000634c != DAT_0011d250) {
            while (traffic_register((void *)0x10100, 0xc, (void *)0x1010c,
                       0x74, DAT_0000634c) == 0) {
            }
        }
        DAT_0000634c++;
    }
}
