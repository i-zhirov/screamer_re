/* The rebuilt `stream_status` module.
 *
 * The original sits at linear 0x4c7e0 in STARTH.EXE and holds 21 instructions
 * and 38 bytes: the six register saves, the frame, the two local slots, the
 * word read at `stream + 0x70`, and the epilogue.  The function returns the
 * status word of a stream record.
 */
unsigned stream_status(void *stream)
{
    return *(unsigned int *)((char *)stream + 0x70);
}
