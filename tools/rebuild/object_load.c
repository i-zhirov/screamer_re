/* The rebuilt `object_load` module.
 *
 * The original sits at linear 0x10bfc in STARTH.EXE, so the plan row records a
 * 232-byte first block of 69 instructions and 5 calls.  The function continues
 * past the early return to the full 728-byte extent: the second block clears
 * the six list words, stores the loaded file in the handle table, binds the
 * block object, and links the chunk table.
 *
 * The function walks the 0x1c-byte records from `start` to `start + count`.
 * The load flag at `+0x14` equal to 1 selects a record.  For a selected record
 * it builds the name `%s.3DX`, copies the name to `DAT_00007fe8`, and opens the
 * member through `fopen` with the base path `DAT_00007f80` and the mode `rb`.
 * It reads the file into the `3D_HEADER` resource, derives the object count at
 * `+0x14`, reads the `3D_BLKOBJ` resource with the size `count * 0x20 + 0x60`,
 * clears the six list words at `+0x24`, `+0x28`, `+0x2c`, `+0x30`, `+0x34`, and
 * `+0x38`, and binds the block object through `object_init_from_file`.
 *
 * The function returns 0 on success, 2 when the open fails, 3 when the
 * `3D_HEADER` resource is missing, and 5 when the `3D_BLKOBJ` resource is
 * missing.
 */
extern int sprintf(char *buffer, const char *format, ...);
extern char *strcpy(char *destination, char *source);
extern int stream_status(char *stream);
extern void *resource_get(void *resource, char *name, int size);
extern int stream_read(char *stream, char *buffer, int count);
extern int stream_close(char *stream);
extern void object_init_from_file(void *object, int index);
extern int fopen(char *stream, char *path, char *name, char *category);
extern char DAT_00007f80[];
extern char DAT_00007fe8[];
extern int DAT_00007f7c;
extern int *DAT_00021540;
extern void *DAT_0000801c;

int object_load(char *objects, int start, int count)
{
    char name[0x40];
    char stream[0xc0];
    char *record;
    void *file_data;
    void *block;
    int file_size;
    int object_count;
    int block_size;
    int end;
    int index;
    int slot;

    DAT_00007f7c = 0;
    while (DAT_00021540[DAT_00007f7c] != -1) {
        DAT_00007f7c++;
    }
    end = start + count;
    index = start;
    while (index < end) {
        record = objects + index * 0x1c;
        if (*(int *)(record + 0x14) == 1) {
            sprintf(name, "%s.3DX", record);
            strcpy(DAT_00007fe8, record);
            if (fopen(stream, DAT_00007f80, name, "rb") != 0) {
                return 2;
            }
            file_size = stream_status(stream);
            file_data = resource_get(DAT_0000801c, "3D_HEADER", file_size);
            if (file_data == 0) {
                return 3;
            }
            stream_read(stream, file_data, file_size);
            stream_close(stream);
            object_count = *(int *)((char *)file_data + 0x14);
            block_size = object_count * 0x20 + 0x60;
            block = resource_get(DAT_0000801c, "3D_BLKOBJ", block_size);
            *(void **)(record + 0xc) = block;
            if (block == 0) {
                return 5;
            }
            *(int *)((char *)block + 0x24) = 0;
            *(int *)((char *)block + 0x28) = 0;
            *(int *)((char *)block + 0x2c) = 0;
            *(int *)((char *)block + 0x30) = 0;
            *(int *)((char *)block + 0x34) = 0;
            *(int *)((char *)block + 0x38) = 0;
            DAT_00021540[DAT_00007f7c] = (int)file_data;
            *(void **)(record + 0x14) = file_data;
            slot = DAT_00007f7c;
            DAT_00007f7c = slot + 1;
            DAT_00021540[slot + 1] = -1;
            object_init_from_file(block, slot);
            *(void **)(record + 0x10) = (char *)block + 0x40;
        }
        index++;
    }
    return 0;
}
