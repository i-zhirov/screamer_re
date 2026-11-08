/* The rebuilt `overlay_queue_update` module.
 *
 * The original sits at linear 0x289bc in STARTH.EXE and holds 117 instructions
 * and 415 bytes up to the `ret` at 0x28b5a.  The merged store record keeps the
 * 416-byte extent.  The body holds the register saves, the frame, the 0xc-byte
 * local block, the two loops, the one call at `0x1f718`, and the epilogue.
 *
 * The first loop walks the signed source list `DAT_00102108` and stops at the
 * first negative entry.  An entry with the value 0 or with a clear byte at
 * `DAT_00100002[i]` is ready.  A ready entry with the flag `DAT_00120884[i]`
 * set to 1 clears the flag and moves into the queue.  The queue write uses the
 * count `DAT_0012087c` and the queue table `DAT_0012082c`: it stores the source
 * value at `queue[count]` and clears `queue[count + 1]`.  The count stops at 4,
 * so the table holds five entries.  A source entry that stays above 0 and has a
 * set byte at `DAT_00100002[i]` sets the flag to 1 and waits.
 *
 * The second loop walks the nine 0x18-byte records of the table at
 * `DAT_00102e04`.  A record holds the five-byte key at `+0`, the value pointer
 * at `+0xc`, the value at `+0x10`, and the xor flag at `+0x14`.  The loop
 * compares the low byte of each of the five queue entries with the key byte.
 * A zero queue entry stops the compare without a mismatch.  A full queue that
 * matches every key byte runs the record: a clear xor flag stores the value
 * through the pointer, and a set xor flag xors the value through the pointer.
 * The code then calls `table_lookup` at `0x1f718` with 8 and clears the count
 * and the first queue entry.  When all nine records mismatch, the code clears
 * the count and the first queue entry.
 *
 * The original table holds the key `-----` in every recovered record.  The
 * table and the queue live in object 3, so the loader does not relocate them.
 * The rebuild reads the table through the global `DAT_00102e04` and the harness
 * fills it.  The original has the EBP frame, so the module builds with `-of+`.
 * The module has no floating-point operation, so it uses no `fpu_stub.asm`.
 */
struct queue_record {
    char key[5];
    char pad[7];
    int *value_ptr;
    int value;
    int xor_flag;
};

extern int DAT_00102108[];
extern unsigned char DAT_00100002[];
extern int DAT_00120884[];
extern int DAT_0012087c;
extern int DAT_0012082c[];
extern struct queue_record DAT_00102e04[];
extern int table_lookup(int index);

void overlay_queue_update(void)
{
    int i;
    int j;
    int index;
    int mismatches;

    for (i = 0; DAT_00102108[i] >= 0; i++) {
        if (DAT_00102108[i] == 0 || DAT_00100002[i] == 0) {
            if (DAT_00120884[i] == 1) {
                DAT_00120884[i] = 0;
                index = DAT_0012087c;
                DAT_0012082c[index] = DAT_00102108[i];
                DAT_0012082c[index + 1] = 0;
                if (index < 4) {
                    DAT_0012087c = index + 1;
                }
            }
        } else {
            DAT_00120884[i] = 1;
        }
    }

    mismatches = 0;
    for (i = 0; i < 9; i++) {
        for (j = 0; j < 5; j++) {
            if (DAT_0012082c[j] == 0) {
                break;
            }
            if ((unsigned char)DAT_00102e04[i].key[j]
                != (unsigned char)DAT_0012082c[j]) {
                mismatches = mismatches + 1;
                break;
            }
        }
        if (j == 5) {
            if (DAT_00102e04[i].xor_flag != 0) {
                *DAT_00102e04[i].value_ptr =
                    *DAT_00102e04[i].value_ptr ^ DAT_00102e04[i].value;
            } else {
                *DAT_00102e04[i].value_ptr = DAT_00102e04[i].value;
            }
            table_lookup(8);
            DAT_0012087c = 0;
            DAT_0012082c[0] = 0;
        }
    }

    if (mismatches == 9) {
        DAT_0012082c[0] = 0;
        DAT_0012087c = 0;
    }
}
