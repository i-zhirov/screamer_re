/* The rebuilt `map_object_line_select` module.
 *
 * The original sits at linear 0x2b308 in STARTH.EXE and holds 66 instructions
 * and 218 bytes up to the `ret` at 0x2b3e1.  The store record keeps the
 * 221-byte extent and sets no function size.  The identity check uses the plan
 * count 66.
 *
 * The two direct callers are `menu_text_update` at 0x29918 and
 * `name_entry_update` at 0x2ce84.  Both keep the 0x50 map object records at
 * `DAT_001db78`, stride 0x64.  A record holds the key at `+0x04` and the
 * animation flag at `+0x0c`.  The caller passes the code in `eax`.  The
 * function sets the flag to zero on the records whose key equals the code, and
 * to one on the other records.  It leaves a flag that already holds the target
 * value unchanged.
 *
 * The original pushes `ebx`, `edx`, `esi`, and `ebp`, and pops them in the
 * reverse order.  The `edx` save lets `name_entry_update` keep its loop counter
 * in `edx` across the call.  The original has the EBP frame, so the module
 * builds with `-of+`.
 */
#define RECORD_SIZE 0x64
#define RECORD_COUNT 0x50
#define KEY_OFFSET 0x04
#define FLAG_OFFSET 0x0c

extern char DAT_001db78[];

void map_object_line_select(int code)
{
    int index;

    for (index = 0; index < RECORD_COUNT; index++) {
        char *record = DAT_001db78 + index * RECORD_SIZE;

        if (*(int *)(record + KEY_OFFSET) == code) {
            if (*(int *)(record + FLAG_OFFSET) != 0)
                *(int *)(record + FLAG_OFFSET) = 0;
        } else {
            if (*(int *)(record + FLAG_OFFSET) != 1)
                *(int *)(record + FLAG_OFFSET) = 1;
        }
    }
}
