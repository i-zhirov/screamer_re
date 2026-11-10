/* The rebuilt `member_load` module.
 *
 * The original sits at linear 0x4c485 in STARTH.EXE and holds 32 instructions
 * and 96 bytes up to the `ret` at 0x4c4e4.  The next function starts at
 * 0x4c4e5, so the store record keeps the 96-byte extent and sets no separate
 * function size.  The identity check uses 32 instructions.
 *
 * The two direct callers pass a stream record in eax, the member path in edx,
 * the member name in ebx, and the destination buffer in ecx.  The caller at
 * 0x4cf3c builds the `%s.COL` palette name and loads it into the palette
 * buffer.  The caller at 0x25aac loads a member into the engine buffer at
 * DAT_000bdd20.  Both callers use the same register order as `fopen`, so the
 * record takes the section Resource loader.
 *
 * The function does three steps:
 *
 *     1  it opens the member through `fopen` at 0x4c08d with the mode `rb`
 *        at object-3 offset 0x16bb
 *     2  it reads the member size bytes from `stream + 0x70` into the
 *        destination buffer through `stream_read` at 0x4c4e5
 *     3  it closes the stream through `stream_close` at 0x4c38d and returns 0
 *
 * A failed open returns -1 and calls no other helper.  The member size sits at
 * `stream + 0x70`.  The function uses no resource layer and no out cell, so it
 * loads a raw member into the caller buffer.
 *
 * The `%s.COL` and engine member callers, the raw destination buffer, and the
 * three stream helpers support the name `member_load`.  The original has no
 * symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame and spills the four register arguments, so
 * the module builds with `-of+`.  The function uses no floating-point
 * instruction, so the module needs no FPU stub, and the module uses no
 * assembly.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero, so the
 * harness build defines `MEMBER_LOAD_HARNESS` and the function builds the
 * `rb` text in a local buffer.  The default build keeps the literal of the
 * original for the later period build.
 */
extern int fopen(char *stream, char *path, char *name, char *category);
extern int stream_read(char *stream, char *buffer, int count);
extern int stream_close(char *stream);

int member_load(char *stream, char *path, char *name, char *buffer)
{
#ifdef MEMBER_LOAD_HARNESS
    char mode_text[4];

    mode_text[0] = 'r';
    mode_text[1] = 'b';
    mode_text[2] = 0;
#else
    char *mode_text = "rb";
#endif

    if (fopen(stream, path, name, mode_text) != 0) {
        return -1;
    }
    stream_read(stream, buffer, *(int *)(stream + 0x70));
    stream_close(stream);
    return 0;
}
