/* The rebuilt `engine_interface_init` module.
 *
 * The original sits at linear 0x10810 in STARTH.EXE and holds 51 instructions
 * and 183 bytes: the frame, the two argument slots, the loop index slot, the
 * `memory_alloc` call at `0x30050`, the `list_create` call at `0x2ecf8`, the
 * loop over 0x78 records of 0x1c bytes, the three field clears, the sentinel
 * store -1, the name buffer clear at `0x7fe8`, the state word clear, and the
 * epilogue.
 *
 * The function allocates the 0x4c-byte engine interface object and creates its
 * 0xb4-record list when the allocation succeeds.  It stores the list pointer
 * at `DAT_0000801c`.  It clears the fields +0x0c, +0x10, and +0x14 and sets the
 * field +0x18 to -1 of the 0x78 records at the first argument.  It clears the
 * name buffer `DAT_00007fe8` and the state word at the second argument.
 */
struct interface_record {
    int field_00;
    int field_04;
    int field_08;
    int field_0c;
    int field_10;
    int field_14;
    int field_18;
};

extern int DAT_0000801c;
extern char DAT_00007fe8;
extern void *memory_alloc(unsigned int size);
extern void *list_create(void *list, int count);

void engine_interface_init(struct interface_record *records, int *state)
{
    int index;
    void *object;

    object = memory_alloc(0x4c);
    if (object != 0) {
        object = list_create(object, 0xb4);
    }
    DAT_0000801c = (int)object;
    for (index = 0; index < 0x78; ++index) {
        records[index].field_14 = 0;
        records[index].field_0c = 0;
        records[index].field_10 = 0;
        records[index].field_18 = -1;
    }
    DAT_00007fe8 = 0;
    *state = 0;
}
