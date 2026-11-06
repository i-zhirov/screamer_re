/* The rebuilt `video_buf_load` module.
 *
 * The original sits at linear 0x4c8f0 in STARTH.EXE and holds 18 instructions
 * and 53 bytes: the register saves, the frame, the three arguments of the
 * resource lookup, the call at `0x2eed0`, the two pointer stores, the null
 * guard, the release call at `0x2edad`, and the epilogue.
 *
 * The function looks the `VIDEO_BUF` resource up through `resource_get`,
 * stores the returned pointer in `DAT_00021504` and `DAT_00121528`, and
 * releases the stream when the lookup fails.  The two lookup constants
 * `0x16c4` and `0x4b500` are the resource key and the name table address of
 * the original image.
 */
extern void *resource_get(void *stream, unsigned key, void *name);
extern void stream_release(void *stream);
extern void *DAT_00021574;
extern void *DAT_00021504;
extern void *DAT_00121528;

void video_buf_load(void)
{
    void *ptr;

    ptr = resource_get(DAT_00021574, 0x16c4, (void *)0x4b500);
    DAT_00021504 = ptr;
    DAT_00121528 = ptr;
    if (ptr == 0) {
        stream_release(DAT_00021574);
    }
}
