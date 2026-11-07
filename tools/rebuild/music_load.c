/* The rebuilt `music_load` module.
 *
 * The original sits at linear 0x2fcec in STARTH.EXE and holds 99 instructions
 * and 379 bytes up to the `ret` at 0x2fe66.  The next function starts at
 * 0x2fe68, so the store record keeps the 380-byte extent and sets no function
 * size.  The identity check uses 99 instructions.
 *
 * Two rebuilt callers name this function `music_load`: `menu_space_init` at
 * 0x29d08 and `race_data_load` at 0x131a8.  Both build a member name such as
 * `music1` and call this function with the music record and the name.  The
 * record gate sits at `+0x14`, the result at `+0x10`, the success flag at `+4`,
 * the path at `+0x18`, and the resource at `+0x98`.
 *
 * The function first stops the current bank through `music_bank_stop`.  It then
 * builds the file name `%s.HMP` from the argument and opens the member through
 * `fopen` with the mode `rb`.  A failed open runs `file_error_report`, which
 * exits.  The function reads the member size from `stream_status`, requests the
 * `MIDI_MUSIC` resource through `resource_get`, and a zero result runs
 * `memory_error_report`, which exits.
 *
 * The function reads the member into the resource buffer through `stream_read`,
 * keeps the size after the read, and closes the stream.  It fills the 16-entry
 * table at the module offset 0x2f04 with `-1` and passes the buffer, the record,
 * and the table to the song loader at 0xd3035.
 *
 * A non-zero loader result frees the `MIDI_MUSIC` resource through
 * `resource_free_name` and sets the record result to 4.  A zero result sets the
 * success flag and the record result to 0.  The function returns the record
 * result.
 *
 * The name `midi_song_load` for the callee at 0xd3035 is inferred, because the
 * original has no symbol.  The caller name `music_load` comes from the two
 * rebuilt callers.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * uses no floating-point instruction, so it needs no FPU stub.
 */
extern void music_bank_stop(char *record);
extern int sprintf(char *buffer, const char *format, ...);
extern int fopen(char *stream, char *path, char *name, char *category);
extern void file_error_report(char *stream);
extern unsigned int stream_status(char *stream);
extern void *resource_get(void *resource, char *name, int size);
extern void memory_error_report(void *resource);
extern int stream_read(char *stream, char *buffer, int count);
extern int stream_close(char *stream);
extern int midi_song_load(void *buffer, char *record, int *table);
extern void resource_free_name(void *resource, char *name);

static int music_read_size;

int music_load(char *record, char *name)
{
    char path[0x80];
    char stream[0xb4];
    int table[16];
    int index;
    int status;
    void *buffer;

    if (*(int *)(record + 0x14) == 0) {
        *(int *)(record + 0x10) = 0;
        return *(int *)(record + 0x10);
    }

    music_bank_stop(record);
    sprintf(path, "%s.HMP", name);
    if (fopen(stream, record + 0x18, path, "rb") != 0) {
        file_error_report(stream);
    }

    status = (int)stream_status(stream);
    buffer = resource_get(*(void **)(record + 0x98), "MIDI_MUSIC", status);
    if (buffer == 0) {
        memory_error_report(*(void **)(record + 0x98));
    }

    stream_read(stream, (char *)buffer, (int)stream_status(stream));
    music_read_size = (int)stream_status(stream);
    stream_close(stream);

    for (index = 0; index < 16; index++) {
        table[index] = -1;
    }

    if (midi_song_load(buffer, record, table) != 0) {
        resource_free_name(*(void **)(record + 0x98), "MIDI_MUSIC");
        *(int *)(record + 0x10) = 4;
        return *(int *)(record + 0x10);
    }

    *(int *)(record + 4) = 1;
    *(int *)(record + 0x10) = 0;
    return *(int *)(record + 0x10);
}
