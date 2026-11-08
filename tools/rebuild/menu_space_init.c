/* The rebuilt `menu_space_init` module.
 *
 * The original sits at linear 0x29d08 in STARTH.EXE and holds 156 instructions
 * and 632 bytes: the five register saves, the frame with the two local name
 * buffers, the `NET_SPACE` resource request at 0x2eed0 with the size 0x2000, the
 * `choice_load` call at 0x1bbc8, the two zero stores at `DAT_0002082c` and
 * `DAT_0002087c`, the `state_cells_set` call at 0x8bd54 with 0xffff54e0, the
 * font table init at 0x25620, the sound slot init at 0x257a0, the rectangle
 * constants at `DAT_00015ce4` through `DAT_00015cf0`, the four `sprintf` calls
 * at 0x4d32c for `%s\gfx\graf.dat`, `menu%c.spr`, `menu%c.fnt`, and
 * `%s\gfx\menu.dat`, the sprite load at 0x257b0, the font load at 0x25658, the
 * cleanup at 0x15150 on a non-zero result, the graphics name copy at 0x10ba4,
 * the material list load at 0x4cd9c, the screen mode set at 0x4c850, the
 * `OPT_PCX` resource request at 0x2eed0 with the size 0x3e800, the `memset` call
 * at 0x8c160, the `video_fill` call at 0x2aa98, the four member list loads at
 * 0x1b394, the two list file loads at 0x15460, the sound bank load at 0x1f2e0,
 * the objects init at 0x2aab8, the `view_move` call at 0x1f94c, the input apply
 * at 0x263cc, the two music calls at 0x2fcec and 0x2febc, and the epilogue.
 *
 * The function loads the network menu resources.  A zero result from either
 * resource request runs the memory error report at 0x2edad, and the function
 * continues.  A non-zero sprite or font load runs the cleanup and the function
 * continues.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern char DAT_00007edc[];
extern void *DAT_000062ac;
extern int DAT_0002082c;
extern int DAT_0002087c;
extern int DAT_00015cf0;
extern int DAT_00015ce4;
extern int DAT_00015ce8;
extern int DAT_00015cec;
extern void *DAT_00021528;
extern void *DAT_0001fad0;
extern void *DAT_0001fb70;
extern void *DAT_0001fb74;
extern void *DAT_0001fb78;
extern void *DAT_0001fb7c;
extern int DAT_0001fb80[];
extern int DAT_0001fd10[];
extern int DAT_0001fea0[];
extern int DAT_00020030[];
extern int DAT_0001fb64;
extern int DAT_0001fb60;
extern int DAT_00018d84;
extern int DAT_0002e04[];
extern int DAT_0015224[];
extern int DAT_0001d2c0;
extern int DAT_0001d2bc;
extern int DAT_0001d2c4;
extern int DAT_0001d2cc;
extern int DAT_00001f04;

extern void *resource_get(void *resource, char *name, int size);
extern void memory_error_report(void *resource);
extern void choice_load(void);
extern void state_cells_set(int value);
extern void font_table_init(void);
extern void sound_slot_init(void);
extern int sprintf(char *buffer, const char *format, ...);
extern int sprite_load(char *file, char *member);
extern void cleanup(void);
extern int font_load(char *file, char *member);
extern void graphics_load(char *name_a, char *name_b);
extern void material_load(char *name);
extern void screen_mode_set(unsigned int width, unsigned int height,
                            unsigned int value);
extern void *memset(void *target, unsigned int count, int value);
extern void video_fill(void);
extern void *member_list_load(char *name, void *array);
extern int list_file_load(char *type, char *name, char *list, void *table,
                          int stride, int count);
extern void sound_bank_load(void);
extern void menu_objects_init(void);
extern void view_move(int a, int b, int c);
extern void input_apply(unsigned int state, void *device);
extern void music_load(char *record, char *name);
extern void music_select(char *record, int value);

void menu_space_init(void)
{
    char member[0x40];
    char path[0x80];

    if (resource_get(DAT_000062ac, "NET_SPACE", 0x2000) == 0) {
        memory_error_report(DAT_000062ac);
    }
    choice_load();
    DAT_0002082c = 0;
    DAT_0002087c = 0;
    state_cells_set(0xffff54e0);
    font_table_init();
    sound_slot_init();
    DAT_00015cf0 = 0x190;
    DAT_00015ce4 = 0;
    DAT_00015ce8 = 0x280;
    DAT_00015cec = 0;
    sprintf(path, "%s\\gfx\\graf.dat", DAT_00007edc);
    sprintf(member, "menu%c.spr", 0x48);
    if (sprite_load(path, member) != 0) {
        cleanup();
    }
    sprintf(member, "menu%c.fnt", 0x48);
    if (font_load(path, member) != 0) {
        cleanup();
    }
    sprintf(path, "%s\\gfx\\menu.dat", DAT_00007edc);
    graphics_load(path, path);
    material_load("menu");
    screen_mode_set(0x140, 0x190, 0xc8);
    DAT_0001fad0 = resource_get(DAT_000062ac, "OPT_PCX", 0x3e800);
    if (DAT_0001fad0 == 0) {
        memory_error_report(DAT_000062ac);
    }
    memset(DAT_00021528, 0x3e800, 0);
    video_fill();
    DAT_0001fb70 = member_list_load("menu", DAT_0001fb80);
    DAT_0001fb74 = member_list_load("cardat", DAT_0001fd10);
    DAT_0001fb78 = member_list_load("trackdat", DAT_0001fea0);
    DAT_0001fb7c = member_list_load("champ", DAT_00020030);
    DAT_0001fb64 = -1;
    DAT_0001fb60 = 0;
    list_file_load("GEN", "MENU", "CHEATS", DAT_0002e04, 0x18, 0xc);
    DAT_00018d84 = list_file_load("GEN", "MENU", "SOUND_FX", DAT_0015224,
        0x24, 0x24);
    sound_bank_load();
    menu_objects_init();
    view_move(DAT_0001d2c0, DAT_0001d2bc, DAT_0001d2c4);
    if (DAT_0001d2cc != 0) {
        input_apply(DAT_00001f04 + 1, (void *)1);
        sprintf(member, "music%d", DAT_00001f04);
        music_load((char *)0x62b0, member);
        music_select((char *)0x62b0, 1);
    }
}
