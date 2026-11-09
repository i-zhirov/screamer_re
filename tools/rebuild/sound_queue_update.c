/* The rebuilt `sound_queue_update` module.
 *
 * The original sits at linear 0x24880 in STARTH.EXE and holds 48 instructions
 * and 152 bytes up to the `ret` at 0x24917.  The next record starts at 0x24918,
 * so the store record keeps the 152-byte extent and sets no separate function
 * size.  The identity check uses 48 instructions.
 *
 * The only direct caller at 0x20024 is race_frame_update at 0x1ff70, so the
 * record takes the section Program functions.  The caller chain, the sound
 * effect queue at DAT_0019080, and the callees support the name
 * sound_queue_update.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The flag DAT_001d2c8 gates the update.  The status DAT_00119070 holds the
 * playing sound or -1.  With the status -1 the update adds the frame delta
 * DAT_00118e08 to the timer DAT_0011907c.  An empty queue, whose head effect
 * DAT_00119080 holds -1, returns at once.
 *
 * Otherwise the update calls `sound_queue_promote`, and when the timer stays
 * below the head delay DAT_00119088 it returns.  It then starts the head entry
 * through `func_24a28`, stores the new status, and on a -1 result pops the
 * queue through `func_24918` and clears the timer.
 *
 * With a playing sound the update calls `func_1f834` with the table id
 * DAT_00119074 and the status.  A zero result keeps the queue, and a non-zero
 * result pops the queue and clears the timer.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds the float add and the float compare, so the shared
 * fpu_stub.asm supplies the FPU run-time symbols.  The module uses no
 * assembly.
 */
extern int DAT_001d2c8;
extern float DAT_00118e08;
extern float DAT_0011907c;
extern int DAT_00119070;
extern int DAT_00119074;
extern float DAT_00119088;
extern int DAT_00119080;

extern void sound_queue_promote(void);
extern int func_24a28(int *entry);
extern void func_24918(void);
extern int func_1f834(int id, int status);

void sound_queue_update(void)
{
    int status;
    int result;

    if (DAT_001d2c8 == 0) {
        return;
    }
    status = DAT_00119070;
    if (status == -1) {
        DAT_0011907c = DAT_00118e08 + DAT_0011907c;
        if (DAT_00119080 == status) {
            return;
        }
        sound_queue_promote();
        if (DAT_0011907c < DAT_00119088) {
            return;
        }
        result = func_24a28(&DAT_00119080);
        DAT_00119070 = result;
        if (result != status) {
            return;
        }
        func_24918();
        DAT_0011907c = 0;
    } else {
        if (func_1f834(DAT_00119074, status) == 0) {
            return;
        }
        func_24918();
        DAT_0011907c = 0;
    }
}
