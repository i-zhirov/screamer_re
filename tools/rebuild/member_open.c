/* The rebuilt `member_open` module.
 *
 * The original sits at linear 0x4c128 in STARTH.EXE and holds 190 instructions
 * and 613 bytes up to the `ret` at 0x4c38c.  The next function starts at
 * 0x4c38d, so the store record keeps the 613-byte extent and sets no separate
 * function size.  The identity check uses 190 instructions.
 *
 * The only direct caller at 0x4c106 is `fopen` at 0x4c08d.  The caller passes
 * the stream record in `eax` and the member name in `edx`, so the record takes
 * the section Resource loader.
 *
 * The directory sits at the end of the container: the function seeks four bytes
 * before the end, reads the directory size, seeks back by the size plus four
 * bytes, and reads 32-byte records forward.  It exclusive-ORs each name byte
 * with 0x3a until a zero byte, and compares the name with the wanted member
 * through `stricmp`.  A non-matching record with a non-empty name reads the
 * next record.  An empty name ends the search.
 *
 * A missing member copies the name to `stream + 0x78`, closes the stream, sets
 * the state at `stream + 0xac` to 2, and returns -1.
 *
 * A found member sets the line flag at `stream + 0x14` from bit 0 of the record
 * flags at `stream + 0x74`, the compression flag at `stream + 0x10` from bit 1,
 * the end at `stream + 0x08` from the record offset plus the end offset, the
 * start at `stream + 0x04`, and the position at `stream + 0x0c`.  It seeks to
 * the start.  When the category at `stream + 0x20` has no `b` and the line flag
 * is clear, it closes the stream and reopens the path at `stream + 0x24` with
 * the category as the mode.  A failed reopen returns -1.
 *
 * When the compression flag is set, the function allocates the 0x8490-byte
 * decoder context through `memory_alloc` and initializes it through `lzw_init`.
 * When the line flag and the compression flag are both set, it allocates the
 * member size plus one bytes through `malloc`, expands the member through
 * `lzw_expand`, and clears `stream + 0x18`.  A failed allocation returns 5.
 * Otherwise it clears the buffer pointer at `stream + 0x1c` and returns 0.
 *
 * The member search, the directory layout, and the caller support the name
 * `member_open`.  The original has no symbol, so the record keeps the name with
 * the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * original uses no floating-point instruction, so the module needs no FPU stub.
 */
extern int fseek(void *stream, long offset, int whence);
extern unsigned int fread(void *pointer, unsigned int size, unsigned int count,
    void *stream);
extern int fclose(void *stream);
extern void *fopen(const char *path, const char *mode);
extern char *strcpy(char *destination, const char *source);
extern int stricmp(const char *left, const char *right);
extern char *strchr(const char *text, int character);
extern void *memory_alloc(unsigned int size);
extern void *malloc(unsigned int size);
extern void *lzw_init(void *context);
extern void lzw_expand(void *context, void *output, int input_limit,
    int output_limit, void *handle);

int member_open(char *stream, char *name)
{
    int directory_size;
    int index;
    void *context;
    void *decoded;
    void *output;

    fseek(*(void **)stream, -4L, 2);
    fread(&directory_size, 4, 1, *(void **)stream);
    fseek(*(void **)stream, -(directory_size + 4), 2);
    fread(stream + 0x58, 0x20, 1, *(void **)stream);

    for (;;) {
        index = 0;
        do {
            stream[0x58 + index] ^= 0x3a;
            if (stream[0x58 + index] == 0) {
                break;
            }
            index++;
        } while (1);

        if (stricmp(stream + 0x58, name) != 0) {
            if (stream[0x58] != 0) {
                fread(stream + 0x58, 0x20, 1, *(void **)stream);
                continue;
            }
        }
        if (stream[0x58] == 0) {
            strcpy(stream + 0x78, name);
            fclose(*(void **)stream);
            *(int *)(stream + 0xac) = 2;
            return -1;
        }
        break;
    }

    *(int *)(stream + 0x14) = *(int *)(stream + 0x74) & 1;
    *(int *)(stream + 0x10) = *(int *)(stream + 0x74) & 2;
    *(int *)(stream + 0x08) =
        *(int *)(stream + 0x68) + *(int *)(stream + 0x70);
    *(int *)(stream + 0x04) = *(int *)(stream + 0x68);
    *(int *)(stream + 0x0c) = *(int *)(stream + 0x04);

    fseek(*(void **)stream, *(int *)(stream + 0x0c), 0);

    if (strchr(stream + 0x20, 'b') == 0 && *(int *)(stream + 0x14) == 0) {
        fclose(*(void **)stream);
        *(void **)stream = fopen(stream + 0x24, stream + 0x20);
        if (*(void **)stream == 0) {
            return -1;
        }
    }

    if (*(int *)(stream + 0x10) != 0) {
        context = memory_alloc(0x8490);
        decoded = context;
        if (context != 0) {
            decoded = lzw_init(context);
        }
        *(void **)(stream + 0xb0) = decoded;
    }

    if (*(int *)(stream + 0x14) != 0 && *(int *)(stream + 0x10) != 0) {
        output = malloc(*(int *)(stream + 0x70) + 1);
        *(void **)(stream + 0x1c) = output;
        if (output == 0) {
            return 5;
        }
        lzw_expand(*(void **)(stream + 0xb0), output,
            *(int *)(stream + 0x6c), *(int *)(stream + 0x70) + 1,
            *(void **)stream);
        *(int *)(stream + 0x18) = 0;
    } else {
        *(void **)(stream + 0x1c) = 0;
    }
    return 0;
}
