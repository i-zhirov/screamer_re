/* The rebuilt `music_bank_load` module.
 *
 * The original starts at linear 0x2f9e0 in STARTH.EXE.  The first block holds
 * 72 instructions and 220 bytes up to the early `ret 4`: the frame, the four
 * argument slots, the fifth argument from the stack, the bank field stores,
 * the name copy, the sound-system call at `0xd25d7`, the zero fill of the
 * 0x24-byte state buffer through `0xd1ec0`, the driver open at `0xd2783`, the
 * cleanup call at `0xd2743`, and the status stores.
 *
 * The merged store record keeps the 688-byte extent, so the function continues
 * after the first return.  A successful driver open opens the member
 * `melodic.bnk` and then `drum.bnk` in the path that the caller built.  The
 * function reads each member into a resource through `0x2efc2`, keeps the two
 * resource pointers in the globals `0x20b6c` and `0x20b74`, and closes each
 * stream.  For the driver values 0xa002 and 0xa009 it installs the two banks
 * through `0xd2ed9`.  A non-zero status releases the two resources and sets
 * the status 3.  The success path sets the active flag at `+0x14` to 1.
 *
 * The original passes two far pointers and a segment to `0xd2783`.  The
 * rebuild passes near pointers, because the module harness and the flat
 * target need no far pointer.  The first block still uses the same argument
 * order and the same status values.
 */
struct music_bank {
    int field_00;
    int field_04;
    int field_08;
    int field_0c;
    int field_10;
    int field_14;
    char path[0x80];
    int resource_set;
};

struct stream {
    char data[0xb4];
};

extern char sound_state[0x24];
extern int music_melodic_data;
extern int music_melodic_size;
extern int music_drum_data;
extern int music_drum_size;
extern int sound_handles[5];

extern int sound_system_init(int a, int b, int c);
extern void *memset(void *dest, int value, unsigned int count);
extern int sound_driver_load(int index, int *record, int *value, void *state,
                             int *bank);
extern void sound_system_cleanup(void);
extern int fopen(void *stream, char *path, char *member, char *category);
extern void file_error_report(void *stream);
extern int stream_status(void *stream);
extern void *resource_read(void *resource, char *name, int size);
extern void memory_error_report(void *resource);
extern int stream_read(void *stream, void *buffer, int size);
extern int stream_close(void *stream);
extern int sound_bank_install(int driver, int flag, void *data);
extern void resource_release(void *resource, char *name);
extern void sound_timer_set(int value);

static void copy_name(char *destination, char *source)
{
    while ((*destination++ = *source++) != 0) {
    }
}

int music_bank_load(void *bank_pointer, int mode, int size, char *name,
                    int resource_set)
{
    struct music_bank *bank;
    struct stream file;
    int value;

    bank = bank_pointer;
    bank->field_04 = -1;
    bank->resource_set = resource_set;
    copy_name(bank->path, name);
    if (sound_system_init(0, 0, 0) != 0) {
        bank->field_10 = 1;
        return bank->field_10;
    }
    memset(sound_state, 0, 0x24);
    value = size;
    if (sound_driver_load(mode, &bank->field_08, &value, sound_state,
                          &bank->field_08) != 0) {
        sound_handles[0] = 0;
        sound_system_cleanup();
        bank->field_10 = 2;
        return bank->field_10;
    }
    if (fopen(&file, bank->path, "melodic.bnk", "rb") != 0) {
        file_error_report(&file);
    }
    music_melodic_data = (int)resource_read((void *)bank->resource_set,
        "MELODIC_BANK", stream_status(&file));
    if (music_melodic_data == 0) {
        memory_error_report((void *)bank->resource_set);
    }
    stream_read(&file, (void *)music_melodic_data, stream_status(&file));
    music_melodic_size = stream_status(&file);
    stream_close(&file);
    if (fopen(&file, bank->path, "drum.bnk", "rb") != 0) {
        file_error_report(&file);
    }
    music_drum_data = (int)resource_read((void *)bank->resource_set,
        "DRUM_BANK", stream_status(&file));
    if (music_drum_data == 0) {
        memory_error_report((void *)bank->resource_set);
    }
    stream_read(&file, (void *)music_drum_data, stream_status(&file));
    music_drum_size = stream_status(&file);
    stream_close(&file);
    if (mode == 0xa002 || mode == 0xa009) {
        bank->field_10 = sound_bank_install(bank->field_08, 1,
            (void *)music_melodic_data);
        bank->field_10 |= sound_bank_install(bank->field_08, 1,
            (void *)music_drum_data);
    }
    if (bank->field_10 != 0) {
        resource_release((void *)bank->resource_set, "MELODIC_BANK");
        resource_release((void *)bank->resource_set, "DRUM_BANK");
        sound_handles[0] = 0;
        sound_system_cleanup();
        bank->field_10 = 3;
        return bank->field_10;
    }
    sound_timer_set(0x7f);
    bank->field_14 = 1;
    return bank->field_10;
}
