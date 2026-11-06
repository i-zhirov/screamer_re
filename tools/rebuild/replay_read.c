/* The rebuilt `replay_read` module.
 *
 * The original sits at linear 0x22e94 in STARTH.EXE and holds 58 instructions
 * and 190 bytes: the three register saves, the frame, the record slot, the
 * eight field copies, the index advance, and the two end-marker tests.
 *
 * The function reads the 20-byte record at
 * `DAT_00118e5c + DAT_00118e64 * 0x14`.  It copies the two dword fields at
 * `+0x00` and `+0x04` and the six signed 16-bit fields at `+0x08`, `+0x0a`,
 * `+0x0c`, `+0x0e`, `+0x10`, and `+0x12` to the replay globals, advances the
 * index, and returns 1 when the new index equals either end marker.
 *
 * The record pointer is volatile.  The original loads the pointer from the
 * frame before every field copy, and the plain local stays in a register in
 * the rebuild.
 */
extern unsigned char *DAT_00118e5c;
extern unsigned int DAT_00118e64;
extern unsigned int DAT_00118e68;
extern unsigned int DAT_00118e70;
extern unsigned int DAT_00118e04;
extern unsigned int DAT_00118e08;
extern int DAT_00119880;
extern int DAT_00119884;
extern int DAT_00119888;
extern int DAT_0011988c;
extern int DAT_00119890;
extern int DAT_00119894;

int replay_read(void)
{
    int * volatile record;

    record = (int *)(DAT_00118e5c + DAT_00118e64 * 0x14);
    DAT_00118e04 = record[0];
    DAT_00118e08 = record[1];
    DAT_00119880 = *(short *)((char *)record + 8);
    DAT_00119884 = *(short *)((char *)record + 0xa);
    DAT_00119888 = *(short *)((char *)record + 0xc);
    DAT_0011988c = *(short *)((char *)record + 0xe);
    DAT_00119890 = *(short *)((char *)record + 0x10);
    DAT_00119894 = *(short *)((char *)record + 0x12);
    ++DAT_00118e64;
    if (DAT_00118e64 == DAT_00118e68 || DAT_00118e64 == DAT_00118e70) {
        return 1;
    }
    return 0;
}
