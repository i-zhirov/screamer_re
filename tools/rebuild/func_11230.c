/* The rebuilt `func_11230` module.
 *
 * The original sits at linear 0x11230 in STARTH.EXE and holds 71 instructions
 * and 202 bytes up to the `ret` at 0x112f9.  The shared epilogue at 0x112fc
 * continues the body to the next entry at 0x11304, so the store record keeps
 * the 212-byte extent and sets no separate function size.  The identity check
 * uses 71 instructions.
 *
 * The only direct caller at 0x14d9c is the replay playback setup at 0x14d2c,
 * which `game_mode_4_start` at 0x12e18 calls at 0x12ed4 after the ghost load at
 * 0x1ba4c.  The setup passes the object-list table `0x6530` in `eax` and the
 * playback block `0x18ed8` in `edx`; the playback block holds the entry index
 * at `+0x00`.
 *
 * The table holds 0x78 records of 0x1c bytes.  The function compares the handle
 * at `+0x14` of the selected record with the handle at `+0x14` of every record.
 * A match on another index marks the handle as shared and stops the scan.  A
 * shared handle runs the one-handle release at 0x11148; any other handle runs
 * the two-handle release at 0x111b4.  A non-zero release result is returned; a
 * zero result becomes -1 for a shared handle and 0 otherwise.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * uses no assembly and no floating-point instruction.
 */
struct object_entry {
    unsigned char flag;
    char pad_01[0x0b];
    int field_0c;
    int field_10;
    int field_14;
    int field_18;
};

extern int release_one(struct object_entry *entry);
extern int release_two(struct object_entry *entry);

int func_11230(struct object_entry *table, int *index)
{
    int shared;
    int i;
    int result;

    shared = 0;
    for (i = 0; i < 0x78; i++) {
        if (table[*index].field_14 == table[i].field_14 && *index != i) {
            shared = 1;
            break;
        }
    }
    if (shared != 0) {
        result = release_one(&table[*index]);
        if (result != 0) {
            return result;
        }
        return -1;
    }
    result = release_two(&table[*index]);
    if (result != 0) {
        return result;
    }
    return 0;
}
