/* The rebuilt `music_bank_reset` module.
 *
 * The original sits at linear 0x2f9b0 in STARTH.EXE and holds 14 instructions
 * and 46 bytes: the frame, the argument slot, the three field stores, and the
 * epilogue.  The function clears the fields `+0x14` and `+0x10` and sets the
 * field `+0x04` to `0xffffffff` in the music bank record.
 */
void music_bank_reset(void *bank)
{
    *(unsigned int *)((char *)bank + 0x14) = 0;
    *(unsigned int *)((char *)bank + 0x10) = 0;
    *(unsigned int *)((char *)bank + 4) = 0xffffffff;
}
