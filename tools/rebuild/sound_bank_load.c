/* The rebuilt `sound_bank_load` module.
 *
 * The original starts at linear 0x1f2e0 in STARTH.EXE and holds 268
 * instructions and 1046 bytes up to the `ret` at `0x1f6f5`.  The function
 * returns at once when the setup word at `DAT_00008058` is zero.  Otherwise it
 * builds `%s\data\sound.dat` from the base path at `DAT_00007f44` and walks the
 * `DAT_00018d84` sample records of 0x24 bytes at `DAT_0015224`.
 *
 * For every record it builds the member `%s.RAW`, opens the path with the
 * category `rb` through the member open at `0x4c08d`, and copies the 0x74-byte
 * record template at object-3 offset 0x1e8c into the record at
 * `DAT_0016034`.  A failed open reports the file error through `0x4bf88` and
 * runs the exit at `0x300be` with the value 0.
 *
 * The function reads the member size through `0x4c7e0`, gets the resource
 * `SOUND_FX` through `0x2eed0`, reads the member through `0x4c4e5`, stores the
 * size, and closes the stream through `0x4c38d`.  It then copies the record
 * fields at `+0x0c`, `+0x10`, `+0x14`, `+0x18`, `+0x1c`, and `+0x20` into the
 * result fields.  The high nibble of `+0x20` selects the type: 0 gives 0, 1
 * gives 2, 2 gives 1, and 3 gives 3.  The other values keep the template value
 * 2.  The flags at `+0x28` hold 0x40 for a non-zero field `+0x18`, 0x4000 for a
 * non-zero field `+0x10`, and 0x100, 0x400, and 0x200 for the bit 0x10 of the
 * bytes `+0x0f`, `+0x1f`, and `+0x23`.
 *
 * The original stores a far pointer into the pair at `+0x00` and `+0x04`.  The
 * rebuild stores a near pointer and leaves the segment at zero, because the
 * flat target needs no far pointer.  The original has the EBP frame, so the
 * module builds with `-of+`.
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness build defines `SOUND_BANK_LOAD_HARNESS`, so the function builds the
 * constant text in local buffers.  The default build keeps the literals of the
 * original for the later period build.
 */
struct sample_input {
    char name[0x0c];
    unsigned int field_0c;
    unsigned int field_10;
    unsigned int field_14;
    unsigned int field_18;
    unsigned int field_1c;
    unsigned int field_20;
};

struct sample_info {
    void *data;
    int segment;
    int size;
    int field_0c;
    int field_10;
    int field_14;
    int index;
    int pad_1c;
    int pad_20;
    int pad_24;
    int flags;
    int pad_2c;
    int field_30;
    int field_34;
    int field_38;
    int pad_3c;
    int field_40;
    int pad_44[12];
};

struct stream {
    char data[0xb4];
};

extern int DAT_00008058;
extern int DAT_00018d84;
extern void *DAT_000062ac;
extern char DAT_00007f44[];
extern struct sample_input DAT_0015224[];
extern struct sample_info DAT_0016034[];

extern int sprintf(char *buffer, const char *format, ...);
extern int fopen(void *stream, char *path, char *member, char *category);
extern void file_error_report(void *stream);
extern void exit(int code);
extern int stream_status(void *stream);
extern void *resource_get(void *resource, char *name, int size);
extern void memory_error_report(void *resource);
extern int stream_read(void *stream, void *buffer, int size);
extern int stream_close(void *stream);

static const struct sample_info default_info = {
    0, 0, 0, 0, 2, 0x7fff, 0x1000, 0, 0, 0, 0, 0, 0, 0, 0x10000, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

#ifdef SOUND_BANK_LOAD_HARNESS
static void fill(char *text, char a, char b, char c, char d,
                 char e, char f, char g, char h)
{
    text[0] = a;
    text[1] = b;
    text[2] = c;
    text[3] = d;
    text[4] = e;
    text[5] = f;
    text[6] = g;
    text[7] = h;
    text[8] = 0;
}

static void build_default(struct sample_info *info)
{
    char *bytes;
    int index;

    bytes = (char *)info;
    for (index = 0; index < sizeof(struct sample_info); ++index) {
        bytes[index] = 0;
    }
    info->field_10 = 2;
    info->field_14 = 0x7fff;
    info->index = 0x1000;
    info->field_38 = 0x10000;
}
#endif

void sound_bank_load(void)
{
#ifdef SOUND_BANK_LOAD_HARNESS
    char path_format[0x14];
    char member_format[0x10];
    char list_name[0x10];
    char mode_text[0x10];
    struct sample_info default_copy;
#else
    char *path_format = "%s\\data\\sound.dat";
    char *member_format = "%s.RAW";
    char *list_name = "SOUND_FX";
    char *mode_text = "rb";
#endif
    struct stream file;
    struct sample_info *info;
    struct sample_input *sample;
    char member[0x64];
    char path[0x6c];
    int index;
    int size;
    int status;
    int flags;

#ifdef SOUND_BANK_LOAD_HARNESS
    fill(path_format, '%', 's', '\\', 'd', 'a', 't', 'a', '\\');
    fill(path_format + 8, 's', 'o', 'u', 'n', 'd', '.', 'd', 'a');
    fill(path_format + 16, 't', 0, 0, 0, 0, 0, 0, 0);
    fill(member_format, '%', 's', '.', 'R', 'A', 'W', 0, 0);
    fill(list_name, 'S', 'O', 'U', 'N', 'D', '_', 'F', 'X');
    fill(mode_text, 'r', 'b', 0, 0, 0, 0, 0, 0);
    build_default(&default_copy);
#endif

    if (DAT_00008058 == 0) {
        return;
    }
    sprintf(path, path_format, DAT_00007f44);
    for (index = 0; index < DAT_00018d84; ++index) {
        sample = &DAT_0015224[index];
        info = &DAT_0016034[index];
        sprintf(member, member_format, sample->name);
        if (fopen(&file, path, member, mode_text) != 0) {
            file_error_report(&file);
            exit(0);
        }
#ifdef SOUND_BANK_LOAD_HARNESS
        *info = default_copy;
#else
        *info = default_info;
#endif
        size = stream_status(&file);
        info->data = resource_get(DAT_000062ac, list_name, size);
        if (info->data == 0 && info->segment == 0) {
            memory_error_report(DAT_000062ac);
        }
        size = stream_status(&file);
        stream_read(&file, info->data, size);
        info->size = stream_status(&file);
        stream_close(&file);
        info->index = index + 1;
        info->field_14 = (int)(sample->field_0c & 0xffff);
        info->field_38 = (int)(sample->field_1c & 0x0fffffff);
        info->field_0c = (int)sample->field_10;
        info->field_30 = (int)sample->field_14;
        info->field_34 = (int)sample->field_18;
        info->field_40 = (int)(sample->field_20 & 0xffff);
        status = (int)((sample->field_20 >> 24) & 0xf);
        if (status == 0) {
            info->field_10 = 0;
        } else if (status == 1) {
            info->field_10 = 2;
        } else if (status == 2) {
            info->field_10 = 1;
        } else if (status == 3) {
            info->field_10 = 3;
        }
        flags = 0;
        if (sample->field_18 != 0) {
            flags |= 0x40;
        }
        if (sample->field_10 != 0) {
            flags |= 0x4000;
        }
        if ((sample->field_0c >> 24) & 0x10) {
            flags |= 0x100;
        }
        if ((sample->field_1c >> 24) & 0x10) {
            flags |= 0x400;
        }
        if ((sample->field_20 >> 24) & 0x10) {
            flags |= 0x200;
        }
        info->flags = flags;
    }
}
