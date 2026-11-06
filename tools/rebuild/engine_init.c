/* The rebuilt `engine_init` module.
 *
 * The original sits at linear 0x127a0 in STARTH.EXE and holds 71 instructions
 * and 266 bytes: the five register saves, the frame, the 0x80-byte local
 * buffer, the `config_load` call at `0x1bf38`, the six-argument
 * `list_file_load` call at `0x15460`, the `setup_load`, `controls_load`, and
 * `mouse_init` calls, the `dpmi_real_int` call at `0x25db0`, the flag store at
 * `DAT_0016684`, the setup-word guard at `DAT_00008070`, the
 * `sound_driver_set(0)` call at `0x262a8`, the `%s\DATA\MUSIC.DAT` build
 * through `sprintf` at `0x4d32c`, the `music_bank_load` call at `0x2f9e0`, the
 * `music_start` call at `0x2ffc4`, the `palette_build` call at `0x15720`, the
 * `keyboard_hook(1)` call at `0x14e20`, the `engine_interface_init` call at
 * `0x10810` with the record table `0x6530` and the state word `0x6350`, the
 * `stream_layer_init` call at `0x4c810`, the `video_buf_load` call at `0x4c8f0`,
 * the `vesa_set_mode(1)` call at `0x8bdec`, and the three rectangle stores at
 * `DAT_0015444`, `DAT_0015448`, and `DAT_001544c`.
 *
 * The function runs the subsystem setup in order: the configuration, the
 * `GEN` list file with the `MENU` name and the `CD_TRACKS` record into the
 * table at `0x1f00`, the setup, the controls, and the mouse.  It stores the
 * value `dpmi_real_int() != 3` at `DAT_0016684`.  When the setup word
 * `DAT_00008070` is non-zero, it stops the sound driver when the flag is set,
 * builds `%s\DATA\MUSIC.DAT` from the config string at `0x7f44` into the local
 * buffer, clears the flag, loads the music bank into the struct at `0x62b0`
 * with the engine list `DAT_000062ac` as the resource set, and reports a
 * non-zero status through `music_start`.  It then builds the palette, hooks the
 * keyboard with 1, initializes the engine interface, loads the stream layer
 * and the video buffer, sets the VESA mode 1, and stores the 0, 0, and 0x280
 * rectangle values.
 *
 * The string constants stay as explicit data offsets `0x437`, `0x432`,
 * `0x428`, and `0x43b`, as the original uses.  The original has the EBP frame,
 * so the module builds with `-of+`.
 */
extern void config_load(void);
extern void list_file_load(char *type, char *name, char *record, void *table,
                           int stride, int count);
extern void setup_load(void);
extern void controls_load(void);
extern void mouse_init(void);
extern int dpmi_real_int(void);
extern void sound_driver_set(int value);
extern int sprintf(char *buffer, const char *format, ...);
extern int music_bank_load(void *bank, int mode, int size, char *name,
                           int resource);
extern void music_start(void *bank);
extern void palette_build(void);
extern void keyboard_hook(int value);
extern void engine_interface_init(void *records, void *state);
extern void stream_layer_init(void *stream);
extern void video_buf_load(void);
extern void vesa_set_mode(int mode);

extern int DAT_0016684;
extern int DAT_00008070;
extern int DAT_00008078;
extern int DAT_000062ac;
extern int DAT_0015444;
extern int DAT_0015448;
extern int DAT_001544c;

void engine_init(void)
{
    int flag;
    char path[0x80];

    config_load();
    list_file_load((char *)0x437, (char *)0x432, (char *)0x428,
                   (void *)0x1f00, 4, 4);
    setup_load();
    controls_load();
    mouse_init();
    flag = dpmi_real_int() != 3;
    DAT_0016684 = flag;
    if (DAT_00008070 != 0) {
        if (flag != 0) {
            sound_driver_set(0);
        }
        DAT_0016684 = 0;
        sprintf(path, (char *)0x43b, (char *)0x7f44);
        if (music_bank_load((void *)0x62b0, DAT_00008070, DAT_00008078,
                            path, DAT_000062ac) != 0) {
            music_start((void *)0x62b0);
        }
    }
    palette_build();
    keyboard_hook(1);
    engine_interface_init((void *)0x6530, (void *)0x6350);
    stream_layer_init((void *)DAT_000062ac);
    video_buf_load();
    vesa_set_mode(1);
    DAT_001544c = 0;
    DAT_0015444 = 0;
    DAT_0015448 = 0x280;
}
