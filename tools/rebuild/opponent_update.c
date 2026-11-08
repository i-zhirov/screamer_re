/* The rebuilt `opponent_update` module.
 *
 * The original sits at linear 0x20954 in STARTH.EXE and holds 134 instructions
 * and 454 bytes up to the `ret` at 0x20b19.  The merged store record keeps the
 * 456-byte extent.  The body holds the three register saves, the frame, the
 * 0x14-byte local block, the range gate, the record loop, the six calls, and
 * the epilogue.
 *
 * When the flag `DAT_0010cf58` is clear, the function returns at once.  It walks
 * the `DAT_0010f178` records of 0xbc bytes at `DAT_0010f180`.  A record runs
 * when its position at `+0x20` and `+0x28` stays closer than 0xcb20 to the
 * camera position `DAT_00113ccc` and `DAT_00113cd0` on both axes.  A record
 * outside that range also runs when the argument is not zero.
 *
 * A running record calls 0x22378 with `DAT_0010f174` and `+0x44` when the
 * argument is not zero.  It then calls 0x20b1c with the same two values and the
 * `+0xc` float scaled by the frame delta `DAT_00118e04`.  It copies the float
 * at `+0x78` to `+0x30`, copies `+0x54`, `+0x58`, and `+0x5c` to `+0x20`,
 * `+0x24`, and `+0x28`, and writes those three values into the target record
 * at `+8`, `+0xc`, and `+0x10`.
 *
 * The target comes from the pointer table `DAT_0010653c` at the 0x1c-byte
 * entry named by `+0x18`.  The function scales the floats at `+0x2c`, `+0x30`,
 * and `+0x34` by 1024.0 / pi, truncates toward zero, masks with 0x7ff, and
 * writes the three words at `+0x14`, `+0x18`, and `+0x1c`.  The constants
 * arrive through `DAT_00100cd9` and `DAT_00100cdd`, so the harness sets the
 * exact bit patterns.
 *
 * The function then calls 0x21b9c with the record and with 1 when the argument
 * is zero, and it advances by 0xbc bytes.  The calls at 0x4d9f4 truncate toward
 * zero, so the module uses the local truncating FPU stub.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_0010cf58;
extern int DAT_0010f178;
extern unsigned char DAT_0010f180[];
extern unsigned int DAT_0010f174;
extern int DAT_00113ccc;
extern int DAT_00113cd0;
extern float DAT_00118e04;
extern float DAT_00100cd9;
extern float DAT_00100cdd;
extern unsigned int DAT_0010653c[];

extern void opponent_callee_22378(unsigned int state, void *body);
extern void opponent_callee_20b1c(unsigned int state, void *body, float value);
extern void opponent_callee_21b9c(void *record, int idle);

void opponent_update(int value)
{
    char *record;
    char *end;
    int dx;
    int dz;
    int *target;

    if (DAT_0010cf58 == 0) {
        return;
    }
    record = (char *)DAT_0010f180;
    end = record + DAT_0010f178 * 0xbc;
    while (record < end) {
        dx = DAT_00113ccc - *(int *)(record + 0x20);
        dz = DAT_00113cd0 - *(int *)(record + 0x28);
        if (dx < 0) {
            dx = -dx;
        }
        if (dz < 0) {
            dz = -dz;
        }
        if ((dx < 0xcb20 && dz < 0xcb20) || value != 0) {
            if (value != 0) {
                opponent_callee_22378(DAT_0010f174, record + 0x44);
            }
            opponent_callee_20b1c(DAT_0010f174, record + 0x44,
                *(float *)(record + 0xc) * DAT_00118e04);
            *(float *)(record + 0x30) = *(float *)(record + 0x78);
            target = (int *)DAT_0010653c[*(int *)(record + 0x18) * 7];
            *(int *)(record + 0x20) = *(int *)(record + 0x54);
            target[2] = *(int *)(record + 0x20);
            *(int *)(record + 0x24) = *(int *)(record + 0x58);
            target[3] = *(int *)(record + 0x24);
            *(int *)(record + 0x28) = *(int *)(record + 0x5c);
            target[4] = *(int *)(record + 0x28);
            target[5] = (int)(*(float *)(record + 0x2c)
                * DAT_00100cd9 / DAT_00100cdd) & 0x7ff;
            target[6] = (int)(*(float *)(record + 0x30)
                * DAT_00100cd9 / DAT_00100cdd) & 0x7ff;
            target[7] = (int)(*(float *)(record + 0x34)
                * DAT_00100cd9 / DAT_00100cdd) & 0x7ff;
            opponent_callee_21b9c(record, value == 0);
        }
        record = record + 0xbc;
    }
}
