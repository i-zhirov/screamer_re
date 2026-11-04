/* The rebuilt `race_data_load` module.
 *
 * The original sits at linear 0x131a8 in STARTH.EXE.  The body up to the `ret`
 * at 0x1353e holds 236 instructions and 919 bytes.  The store keeps the
 * 920-byte extent and sets no function size.
 *
 * The caller at 0x128e0 is `race_session_start`.  It runs this function as its
 * first step after it sets the flag `_DAT_00118e1c = 0x102`, and it returns the
 * result.  The data flow names the function `race_data_load`: it builds the
 * track file names from the track name `DAT_0011d294`, loads the five lists
 * `OSTACLES`, `TRACK_DATA`, `OPPONENTS`, `ANIMATIONS`, and `CARS_DATA`, and
 * loads the graphics, the fonts, the sprites, and the music of the race.
 *
 * The letter `H` or `L` selects the high or the low asset member.  The index
 * `DAT_0011d2b8 < 1` gives `L`, and every other value gives `H`.  The flag
 * `DAT_0011d274 & 8` adds the suffix `i` to the track name, so the name is
 * `MAP1i` or `MAP1`.  The level `DAT_0011d2b4` gives the member letter
 * `A + level`.  The function reads the lists in this order:
 *
 *     OSTACLES      DAT_0010f000  0x24 records of 0x24 bytes
 *     TRACK_DATA    DAT_0010cf44  0x10 records of 0x10 bytes
 *     OPPONENTS     DAT_00110d9c  0x590 records of 0x14 bytes
 *     ANIMATIONS    DAT_0010f180  0xbc records of 0x18 bytes
 *     CARS_DATA     DAT_0010d25c  0x40 records of 0x40 bytes
 *
 * The record keeps the opponent count in `DAT_0010f8ec`, the animation count in
 * `DAT_0010f17c`, and the car count in `DAT_0010d6dc`.  The first `TRACK_DATA`
 * value is the track color.  The function copies the color to all four bytes of
 * the word at `PTR_DAT_0008a844 + 0x10`.
 *
 * The game mode `DAT_0011d260 == 0x100` loads `attr%c.fnt` and `logo%c.spr`
 * from `%s\gfx\graf.dat`.  Every other mode loads `game%c.fnt` and
 * `game%c.spr` from `%s\gfx\game.dat` and the three resources `impres_`,
 * `implet_`, and `cdfront`.  The flag `DAT_0011d2cc` runs the music load.
 *
 * Inference: the names of the call targets come from the strings and the
 * call graph.  The replay open at 0x1b840 builds `%s\replays\%s.rpl`.  The
 * material calls at 0x4cd9c and 0x4c94c set the track material.  The build at
 * 0x14fb8 writes the names `%sopp`, `%sanm`, and `%scam`.  The loader at
 * 0x150f4 reads the track resources.  The open at 0x25aac opens the `.smg`
 * stream and reports a file error.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern char DAT_00007edc[];
extern char DAT_00007f44[];
extern int DAT_00001f00;
extern int DAT_00001f08[];
extern int DAT_000203bc;
extern int DAT_0010cf44[];
extern int DAT_0010d25c[];
extern int DAT_0010d6dc;
extern int DAT_0010f000[];
extern int DAT_0010f17c;
extern int DAT_0010f180[];
extern int DAT_0010f8ec;
extern char DAT_0011d278[];
extern char DAT_0011d294[];
extern int DAT_00110d9c[];
extern int DAT_00118e30;
extern char DAT_00118f10[];
extern char DAT_00118f14[];
extern char DAT_00118f18[];
extern int DAT_00118f94[];
extern int DAT_0011905c;
extern int DAT_0011d260;
extern int DAT_0011d274;
extern int DAT_0011d2b4;
extern int DAT_0011d2b8;
extern int DAT_0011d2cc;
extern char DAT_001062b0[];
extern unsigned int *PTR_DAT_0008a844;

extern int sprintf(char *buffer, const char *format, ...);
extern void graphics_load(char *first, char *second);
extern void replay_open(char *name, int value);
extern int list_file_load(char *type, char *name, char *list, void *table,
                          int stride, int count);
extern void material_load(char *name);
extern void material_name_set(char *name);
extern void track_stream_open(char *path, char *member, int value);
extern void screen_mode_set(unsigned int width, unsigned int height,
                            unsigned int value);
extern void track_names_build(char *name);
extern void track_resource_load(char *name);
extern void font_table_init(void);
extern void sound_slot_init(void);
extern int font_load(char *file, char *member);
extern int sprite_load(char *file, char *member);
extern int member_list_load(char *name, void *array);
extern void cockpit_resource_load(char *name, void *out);
extern void music_load(void *record, char *name);

int race_data_load(int level)
{
    char path[0x80];
    char type[0x80];
    char member[0x80];
    char data[0x40];
    char letter;
    unsigned int color;

    if (level == 0) {
        return 0;
    }
    letter = DAT_0011d2b8 < 1 ? 0x4c : 0x48;
    sprintf(path, "%s\\gfx\\%s%c.dat", DAT_00007f44, DAT_0011d294, letter);
    graphics_load(path, path);
    if (DAT_00118e30 != 0) {
        replay_open(DAT_0011d278, 0);
    }
    sprintf(type, "%s%s", DAT_0011d294, (DAT_0011d274 & 8) ? "i" : "");
    sprintf(member, "%s%c", DAT_0011d294, 0x41 + DAT_0011d2b4);
    sprintf(data, "%sdat", DAT_0011d294);
    list_file_load(type, data, "OSTACLES", DAT_0010f000, 0x24, 0x24);
    list_file_load(type, member, "TRACK_DATA", DAT_0010cf44, 0x10, 0x10);
    DAT_0010f8ec = list_file_load(type, member, "OPPONENTS", DAT_00110d9c,
        0x590, 0x14);
    DAT_0010f17c = list_file_load(type, data, "ANIMATIONS", DAT_0010f180,
        0xbc, 0x18);
    DAT_0010d6dc = list_file_load("GEN", "CARS", "CARS_DATA", DAT_0010d25c,
        0x40, 0x40);
    material_load(DAT_0011d294);
    material_name_set(DAT_0011d294);
    sprintf(member, "%s.smg", DAT_0011d294);
    track_stream_open(path, member, 0x104);
    screen_mode_set(0x140, 0x190, 0x104);
    color = (unsigned int)DAT_0010cf44[0];
    *(unsigned int *)((char *)PTR_DAT_0008a844 + 0x10) =
        (color << 0x18) | (color << 0x10) | (color << 8) | color;
    track_names_build(type);
    track_resource_load(type);
    font_table_init();
    sound_slot_init();
    if (DAT_0011d260 == 0x100) {
        sprintf(path, "attr%c.fnt", 0x48);
        sprintf(member, "%s\\gfx\\graf.dat", DAT_00007edc);
        font_load(member, path);
        sprintf(path, "logo%c.spr", 0x48);
        sprite_load(member, path);
        DAT_0011905c = member_list_load("game", DAT_00118f94);
        sprintf(path, "game%c.spr", DAT_00001f00);
        if (DAT_0011d2cc == 0) {
            return 0;
        }
    } else {
        sprintf(path, "game%c.fnt", 0x48);
        sprintf(member, "%s\\gfx\\game.dat", DAT_00007f44);
        font_load(member, path);
        sprintf(path, "game%c.spr", 0x48);
        sprite_load(member, path);
        DAT_0011905c = member_list_load("game", DAT_00118f94);
        cockpit_resource_load("impres_", DAT_00118f10);
        cockpit_resource_load("implet_", DAT_00118f18);
        cockpit_resource_load("cdfront", DAT_00118f14);
        if (DAT_0011d2cc == 0) {
            return 0;
        }
        sprintf(path, "music%d", DAT_00001f08[DAT_000203bc]);
    }
    music_load(DAT_001062b0, path);
    return 0;
}
