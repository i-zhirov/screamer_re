/* The rebuilt `track_stream_open` module.
 *
 * The original sits at linear 0x25aac in STARTH.EXE and holds 22 instructions
 * and 65 bytes up to the `ret` at 0x25aec.  The next code starts at 0x25af0,
 * so the store record keeps the 68-byte extent and sets no separate function
 * size.  The identity check uses 22 instructions.
 *
 * The function receives the container path in `eax` and the member name in
 * `edx`.  It loads the engine buffer pointer from the cell `PTR_DAT_000bdd20`,
 * calls `member_load` at 0x4c485 with the local stream record, the path, the
 * name, and the buffer pointer, and a non-zero result runs `file_error_report`
 * at 0x4bf88 with the stream record.  The pointer cell holds the buffer at
 * object-1 offset 0xbdd24, so the load fills the engine buffer.
 *
 * The only direct caller is race_data_load at 0x131a8 with the call site at
 * 0x1336d.  The caller builds the `%s\gfx\%s%c.dat` path and the `%s.smg`
 * member name first, so the function opens the level member `.smg` from the
 * track archive.  The caller and the data flow support the name
 * `track_stream_open` with the confidence inferred.  The function loads a
 * resource, so the record takes the section Resource loader.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses no floating-point instruction and calls no floating-point
 * helper, so the module needs no FPU stub and no assembly implementation.
 */
extern int member_load(char *stream, char *path, char *name, char *buffer);
extern void file_error_report(char *stream);
extern char *PTR_DAT_000bdd20;

void track_stream_open(char *path, char *member, int value)
{
    char stream[0xb4];

    if (member_load(stream, path, member, PTR_DAT_000bdd20) != 0) {
        file_error_report(stream);
    }
}
