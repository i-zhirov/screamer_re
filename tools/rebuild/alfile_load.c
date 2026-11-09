/* The rebuilt `alfile_load` module.
 *
 * The original sits at linear 0x4c3e6 in STARTH.EXE and holds 49 instructions
 * and 159 bytes up to the `ret 4` at 0x4c484.  The next record starts at
 * 0x4c485, so the store record keeps the 159-byte extent and sets no separate
 * function size.  The identity check uses 49 instructions.
 *
 * The only direct caller is the member loader at 0x4cae4.  It builds the
 * `%s.MMP` member name from DAT_002154c, opens the member with the empty path
 * at object-3 offset 0x7fb4, and passes the stream, the path, the name, the
 * out cell DAT_0021468, and the resource head DAT_0021574.  After the call it
 * copies the loaded resource pointer to DAT_00102fa4 and runs the spatial grid
 * relocate at 0x4cb4c.
 *
 * The function does three steps:
 *
 *     1  it opens the member through `fopen` at 0x4c08d with the mode `rb`
 *     2  it gets the `ALFILE` resource of the member size plus 4 through
 *        `resource_get` at 0x2eed0 and stores it in the out cell
 *     3  it reads the member size bytes into the resource through
 *        `stream_read` at 0x4c4e5 and closes the stream through
 *        `stream_close` at 0x4c38d
 *
 * A failed open returns -1 and leaves the out cell unchanged.  A zero resource
 * sets the stream state at +0xac to 3 and returns -1.  A success returns 0.
 * The member size sits at `stream + 0x70`.  The resource request adds 4 bytes,
 * and the read count does not.
 *
 * The `ALFILE` resource name, the `.MMP` member name, and the single caller
 * support the name `alfile_load`.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The original has the EBP frame and spills the four register arguments, so
 * the module builds with `-of+`.  The function uses no floating-point
 * instruction, so the module needs no FPU stub, and the module uses no
 * assembly.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero, so the
 * harness build defines `ALFILE_LOAD_HARNESS` and the function builds the two
 * constant texts in local buffers.  The default build keeps the literals of
 * the original for the later period build.
 */
extern int fopen(char *stream, char *path, char *name, char *category);
extern void *resource_get(void *resource, char *name, int size);
extern int stream_read(char *stream, char *buffer, int count);
extern int stream_close(char *stream);

int alfile_load(char *stream, char *path, char *name, void **out, void *resource)
{
#ifdef ALFILE_LOAD_HARNESS
    char mode_text[4];
    char resource_text[8];

    mode_text[0] = 'r';
    mode_text[1] = 'b';
    mode_text[2] = 0;
    resource_text[0] = 'A';
    resource_text[1] = 'L';
    resource_text[2] = 'F';
    resource_text[3] = 'I';
    resource_text[4] = 'L';
    resource_text[5] = 'E';
    resource_text[6] = 0;
#else
    char *mode_text = "rb";
    char *resource_text = "ALFILE";
#endif

    if (fopen(stream, path, name, mode_text) != 0) {
        return -1;
    }
    *out = resource_get(resource, resource_text,
                        *(int *)(stream + 0x70) + 4);
    if (*out == 0) {
        *(int *)(stream + 0xac) = 3;
        return -1;
    }
    stream_read(stream, (char *)*out, *(int *)(stream + 0x70));
    stream_close(stream);
    return 0;
}
