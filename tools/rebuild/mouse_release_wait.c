/* The rebuilt `mouse_release_wait` module.
 *
 * The original sits at linear 0x1f8dc in STARTH.EXE and holds 37 instructions
 * and 112 bytes up to the `ret` at 0x1f94b.  The next function starts at
 * 0x1f94c, so the store record keeps the 112-byte extent and sets no separate
 * function size.  The identity check uses 37 instructions.
 *
 * The two direct callers are `race_frame_update` at 0x1ff70 and `race_end` at
 * 0x279a4.  The frame update calls this function at 0x1ffa6 when bit 2 of the
 * input byte `DAT_00118e1d` is set, and then clears `DAT_00118e6c`.  The race
 * end calls this function at 0x279b3 after the mouse write at 0x1f8b4.  The
 * callers sit in two sections, so the record takes the section Program
 * functions.
 *
 * The setup word `DAT_00008058` gates the drain.  When the word is zero, the
 * function skips the drain.  Otherwise it counts the active mouse button slots
 * through 0xca915.  While a slot is active it tests the slot through 0xca815
 * and releases each pressed slot through 0xca3c9.  The index advances on each
 * turn and the phase counter `DAT_000007f9` is saved at the end of each turn.
 *
 * The function then waits through the phase delta at 0x1fa04.  The wait ends
 * when the low 16 bits of the delta between the saved phase and the current
 * phase reach 0x1e.  The input drain and the phase wait support the name
 * `mouse_release_wait`.
 *
 * The original has the EBP frame and one local, so the module builds with
 * `-of+`.  The function holds no floating-point instruction, so it needs no
 * FPU stub.  The module uses no assembly body.
 */
extern int DAT_00008058;
extern int DAT_00018d94;
extern int DAT_000007f9;
extern int mouse_button_count(int handle);
extern int mouse_button_free(int handle, int index);
extern int mouse_button_release(int handle, int index);
extern int phase_delta(int start, int now);

void mouse_release_wait(void)
{
    int index;
    int saved;

    if (DAT_00008058 != 0) {
        index = 0;
        while (mouse_button_count(DAT_00018d94) != 0) {
            if (mouse_button_free(DAT_00018d94, index) == 0) {
                mouse_button_release(DAT_00018d94, index);
            }
            index = index + 1;
            saved = DAT_000007f9;
        }
    }
    while ((phase_delta(saved, DAT_000007f9) & 0xffff) < 0x1e) {
    }
}
