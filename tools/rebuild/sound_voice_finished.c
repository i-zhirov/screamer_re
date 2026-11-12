/* The rebuilt `sound_voice_finished` module.
 *
 * The original sits at linear 0x1f834 in STARTH.EXE.  The identity window holds
 * 20 instructions and 59 bytes up to the success-path `ret` at 0x1f86e.  Two
 * return-1 tails follow out of line at 0x1f86f and 0x1f878.  The next function
 * starts at 0x1f880, so the store record keeps the 76-byte extent and sets no
 * separate function size.
 *
 * The only direct caller at 0x248fb is `sound_queue_update` at 0x24880.  The
 * caller passes the sound table id `DAT_00119074` in eax and the playing sound
 * status `DAT_00119070` in edx.  A zero result keeps the queue, and a non-zero
 * result pops the queue and clears the timer.  The caller chain, the table id,
 * and the sibling sound voice functions support the name
 * `sound_voice_finished`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The setup word `DAT_00008058` gates the check.  When the word is zero, the
 * function returns 0.  Otherwise it reads the voice word at driver offset
 * 0x34 through 0xc6bde with the driver handle `DAT_00018d94` and the status.
 * When that word equals the table id plus one, the function returns the free
 * check at 0xca815, which reads bit 0x8000 of the voice word at offset 0x30
 * and returns 1 when the bit is clear.  A different word returns 1.
 *
 * The original has the EBP frame and two locals, so the module builds with
 * `-of+`.  The function holds no floating-point instruction, so the module
 * needs no FPU stub.  The module uses no assembly implementation.
 */
extern int DAT_00008058;
extern int DAT_00018d94;
extern int driver_voice_field(int handle, int voice);
extern int driver_voice_idle(int handle, int voice);

int sound_voice_finished(int table_id, int status)
{
    if (DAT_00008058 == 0) {
        return 0;
    }
    if (driver_voice_field(DAT_00018d94, status) == table_id + 1) {
        return driver_voice_idle(DAT_00018d94, status);
    }
    return 1;
}
