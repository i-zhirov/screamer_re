/* The rebuilt `stream_close` module.
 *
 * The original sits at linear 0x4c38d in STARTH.EXE and holds 34 instructions
 * and 89 bytes: the six register saves, the frame, the stream slot, the two
 * buffer guards, the run-time `free` call at `0xd2008`, the wrapper `free` call
 * at `0x300ec`, the handle close at `0xc6245`, and the zero return.
 *
 * The function releases the buffer at `stream + 0x1c` when it is set, releases
 * the buffer at `stream + 0xb0` when the flag at `stream + 0x10` is set, closes
 * the handle at `stream + 0x00`, and returns 0.  It is the counterpart of
 * `fopen`.
 */
extern void nfree(void *p);
extern void free(void *p);
extern int handle_close(int handle);

int stream_close(char *stream)
{
    void *buffer;

    buffer = *(void **)(stream + 0x1c);
    if (buffer != 0) {
        nfree(buffer);
    }
    if (*(int *)(stream + 0x10) != 0) {
        free(*(void **)(stream + 0xb0));
    }
    handle_close(*(int *)stream);
    return 0;
}
