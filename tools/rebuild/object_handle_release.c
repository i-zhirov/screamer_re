/* The rebuilt `object_handle_release` module.
 *
 * The original sits at linear 0x11148 in STARTH.EXE and holds 13 instructions
 * and 29 bytes up to the early `ret` at 0x11164.  The full body continues at
 * 0x11165 through the second return to the `ret` at 0x111b2.  The next recorded
 * function `object_handle_pair_release` starts at 0x111b4, so the store record
 * keeps the 108-byte extent and sets no separate function size.  The identity
 * check uses 13 instructions.
 *
 * The only direct caller is `func_11230` at 0x11230 with the call site at
 * 0x112b3.  The caller selects the object-list entry by the playback index and
 * passes the entry in `eax`.  A handle at `+0x14` shared with another entry runs
 * this function.  Any other handle runs the two-handle release at 0x111b4, so
 * the two functions form the one-handle and two-handle release pair.
 *
 * The entry holds the flag byte at `+0x00`, the first handle at `+0x0c`, the
 * field `+0x10`, the second handle at `+0x14`, and the field `+0x18`.  The
 * function returns 9 when the first handle is zero and returns 0xb when the
 * field `+0x18` is not -1.  Otherwise it frees the first handle from the
 * resource list `DAT_0000801c` through `resource_find` at 0x2f25f.  It then
 * clears the flag byte and the fields `+0x0c`, `+0x10`, and `+0x14`, and it
 * returns 0.  The field `+0x18` stays unchanged.
 *
 * The caller, the field layout, and the release pair support the name
 * `object_handle_release`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.  The function sits with the object
 * handles, so it takes the section Resource loader.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly body.
 */
struct object_entry {
    unsigned char flag;
    char pad_01[0x0b];
    int field_0c;
    int field_10;
    int field_14;
    int field_18;
};

extern int DAT_0000801c;

extern int resource_find(int context, int key);

int object_handle_release(struct object_entry *entry)
{
    if (entry->field_0c == 0) {
        return 9;
    }
    if (entry->field_18 != -1) {
        return 0xb;
    }
    resource_find(DAT_0000801c, entry->field_0c);
    entry->flag = 0;
    entry->field_0c = 0;
    entry->field_10 = 0;
    entry->field_14 = 0;
    return 0;
}
