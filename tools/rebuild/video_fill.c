/* The rebuilt `video_fill` module.
 *
 * The original sits at linear 0x2aa98 in STARTH.EXE and holds 12 instructions
 * and 30 bytes: the register saves `ebx` and `edx`, the frame `push ebp` and
 * `mov ebp,esp`, the size pair `edx=0x190` (400) and `eax=0x1e0` (480), the
 * buffer pointer loaded from `DAT_00121528` into `ebx`, the call to the video
 * copy at `0x8beb8`, and the epilogue `pop ebp`, `pop edx`, `pop ebx`, and
 * `ret`.
 *
 * The callee at `0x8beb8` copies the buffer to the VGA display at `0xa0000`
 * through the VBE bank switch at `0x8c0fc`.  The pointer `DAT_00121528` holds
 * the video buffer address: `video_buf_load` at `0x4c8f0` stores the loaded
 * `VIDEO_BUF` resource pointer there, and `frame_update` clears the buffer
 * through the same pointer.
 *
 * The game files passed the Watcom stack-frame option `-of+`, so the rebuild
 * adds it too.  OpenWatcom V2 cannot reproduce the original code shape in C:
 * it saves the registers after the frame and always emits an ESP restore, so
 * the rebuild holds one extra `lea esp,[ebp-8]` and the identity check stays
 * open.  The alternative assembly build at `video_fill.asm` holds the original
 * shape exactly and passes the identity check, and a build with the original
 * Watcom 10.x compiler should reproduce the C code shape.
 */
extern void video_copy(unsigned int width, unsigned int height, void *source);
extern void *DAT_00121528;

void video_fill(void)
{
    video_copy(480, 400, DAT_00121528);
}
