/* The rebuilt `replay_write` module.
 *
 * The original sits at linear 0x22f54 in STARTH.EXE and holds 64 instructions
 * and 228 bytes: the five register saves, the frame, the record slot, the two
 * float copies, the six field copies, the index advance, the wrap, and the two
 * end-marker tests.
 *
 * The function writes the 20-byte record at
 * `DAT_00118e5c + DAT_00118e64 * 0x14`.  It copies the two float fields at
 * `+0x00` and `+0x04` and the six signed 16-bit fields at `+0x08`, `+0x0a`,
 * `+0x0c`, `+0x0e`, `+0x10`, and `+0x12` from the replay globals.  It advances
 * the index and stops one short of the end marker.  The second end marker
 * `DAT_00118e70` becomes `DAT_00118e68 - 2` when the index reaches the last
 * slot, and equals the index otherwise.
 *
 * The record pointer is volatile.  The original loads the pointer from the
 * frame before every field copy, and the plain local stays in a register in
 * the rebuild.
 */
extern unsigned char *DAT_00118e5c;
extern unsigned int DAT_00118e60;
extern unsigned int DAT_00118e64;
extern unsigned int DAT_00118e68;
extern unsigned int DAT_00118e70;
extern float DAT_00118e04;
extern float DAT_00118e08;
extern short DAT_00119880;
extern short DAT_00119884;
extern short DAT_00119888;
extern short DAT_0011988c;
extern short DAT_00119890;
extern short DAT_00119894;

void replay_write(void)
{
    int * volatile record;

    if (DAT_00118e60 == 0) {
        return;
    }
    record = (int *)(DAT_00118e5c + DAT_00118e64 * 0x14);
    *(float *)((char *)record + 0x00) = DAT_00118e04;
    *(float *)((char *)record + 0x04) = DAT_00118e08;
    *(short *)((char *)record + 0x08) = DAT_00119880;
    *(short *)((char *)record + 0x0a) = DAT_00119884;
    *(short *)((char *)record + 0x0c) = DAT_00119888;
    *(short *)((char *)record + 0x0e) = DAT_0011988c;
    *(short *)((char *)record + 0x10) = DAT_00119890;
    *(short *)((char *)record + 0x12) = DAT_00119894;
    ++DAT_00118e64;
    if (DAT_00118e64 == DAT_00118e68) {
        --DAT_00118e64;
    }
    if (DAT_00118e64 == DAT_00118e68 - 1) {
        DAT_00118e70 = DAT_00118e68 - 2;
    } else {
        DAT_00118e70 = DAT_00118e64;
    }
}
