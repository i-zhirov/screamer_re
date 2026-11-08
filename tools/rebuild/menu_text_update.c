/* The rebuilt `menu_text_update` module.
 *
 * The original sits at linear 0x29918 in STARTH.EXE and holds 174 instructions
 * and 657 bytes up to the `ret` at 0x29ba8.  The next function starts at
 * 0x29bac, so the store record keeps the 660-byte extent and sets no function
 * size.
 *
 * The only direct caller is case 0 of `race_state_update` at 0x29564, which
 * calls this function at 0x29581.  The function places the text lines of the
 * selected menu into the 0x50 map object records and animates them, so the
 * module records the name `menu_text_update`.
 *
 * The function compares the current menu key `DAT_0011fb64` with the selected
 * menu `DAT_0011fb60`.  When the two differ it resets the 0x50 records at
 * `DAT_001db78` through 0x2b450 and counts the active lines of the menu table
 * `DAT_002de8[DAT_0011fb60]` into `DAT_0011fb6c`.  Each 16-byte line record
 * holds an offset at `+0x00` and an offset at `+0x08`, and their sum indexes
 * the string pointer table `DAT_001fb80`.
 *
 * The function fills the line descriptor at `DAT_0011fad4` and calls
 * `map_object_place` at 0x2b044 on each active line.  The descriptor is the
 * same 0x30-byte structure that `map_object_place` reads: the line, the row,
 * the centered flag, the selection flag, the two start positions, the third
 * start position, the two amplitudes, the angle 0.09, and the timer.  The line
 * id rises by one and the row rises by two per line.  The five start and
 * amplitude values come from the 8-entry table at 0x221c, indexed by the
 * counter `DAT_0011fabc`.
 *
 * The function then marks the selected line through 0x2b308 and advances the
 * wave phase `DAT_0011d248` by the frame delta `DAT_0018e08` times 3 pi.  A
 * phase above 8 pi resets to zero.
 *
 * When the key `DAT_0011fb64` is -2 the function runs 0x2b494 on every record
 * and restores the key.  Otherwise it runs `map_object_update` at 0x2b9cc on
 * every record and stores the current key when the update leaves the key
 * unchanged.
 *
 * The float constants arrive through globals, because the minimal image does
 * not load the constant segment at linear 0xf3b and 0xf3f.  The fixed angle
 * 0.09 is a literal.  The original has the EBP frame, so the module builds
 * with `-of+`.
 */
extern char *DAT_002de8[];
extern int DAT_001fb08[];
extern char *DAT_001fb80[];
extern char DAT_001db78[];
extern int DAT_0011fb60;
extern int DAT_0011fb64;
extern int DAT_0011fb6c;
extern int DAT_0011fabc;
extern int DAT_0011fac0;
extern int DAT_0000221c[];
extern float DAT_0018e08;
extern float DAT_0011d248;
extern float menu_text_phase_rate;
extern float menu_text_phase_bound;

struct map_line {
    int line;
    int field4;
    int field8;
    int centered;
    int flag10;
    int start_x;
    int start_y;
    int start_z;
    int amplitude_x;
    int amplitude_y;
    float angle;
    float timer;
};

extern struct map_line DAT_0011fad4;

extern void map_object_record_init(char *record, int line);
extern void map_object_place(struct map_line *line, char *text);
extern void map_object_line_select(int entry);
extern void map_object_update(char *record);
extern void map_object_finish(char *record);

void menu_text_update(void)
{
    char *table;
    char *record;
    int index;
    int value;

    if (DAT_0011fb64 != DAT_0011fb60) {
        for (index = 0; index < 0x50; index++) {
            record = DAT_001db78 + index * 0x64;
            map_object_record_init(record, index);
        }

        DAT_0011fb6c = 0;
        for (index = 0; ; index++) {
            DAT_0011fb6c = index;
            table = DAT_002de8[DAT_0011fb60];
            if (*(int *)(table + index * 0x10) == -1) {
                break;
            }
            DAT_0011fb6c = DAT_0011fb6c + 1;
        }

        DAT_0011fad4.centered = 1;
        DAT_0011fad4.timer = 0.0f;
        DAT_0011fad4.line = 0;
        DAT_0011fad4.field4 = 8 - DAT_0011fb6c;

        value = DAT_0011fabc;
        DAT_0011fad4.start_x = DAT_0000221c[value * 5];
        DAT_0011fad4.start_y = DAT_0000221c[value * 5 + 1];
        DAT_0011fad4.start_z = DAT_0000221c[value * 5 + 2];
        DAT_0011fad4.amplitude_x = DAT_0000221c[value * 5 + 3];
        DAT_0011fad4.amplitude_y = DAT_0000221c[value * 5 + 4];
        DAT_0011fad4.angle = 0.09f;

        for (index = 0; ; index++) {
            table = DAT_002de8[DAT_0011fb60];
            if (*(int *)(table + index * 0x10) == -1) {
                break;
            }
            DAT_0011fad4.flag10 =
                (index != DAT_001fb08[DAT_0011fb60]);
            map_object_place(&DAT_0011fad4,
                DAT_001fb80[*(int *)(table + index * 0x10)
                    + *(int *)(table + index * 0x10 + 8)]);
            DAT_0011fad4.line = DAT_0011fad4.line + 1;
            DAT_0011fad4.field4 = DAT_0011fad4.field4 + 2;
        }

        value = DAT_0011fabc + 1;
        DAT_0011fabc = value;
        if (value >= 8) {
            DAT_0011fabc = 0;
        }
    }

    map_object_line_select(DAT_001fb08[DAT_0011fb60]);

    DAT_0011d248 = DAT_0011d248 + DAT_0018e08 * menu_text_phase_rate;
    if (DAT_0011d248 > menu_text_phase_bound) {
        DAT_0011d248 = 0.0f;
    }

    if (DAT_0011fb64 == -2) {
        for (index = 0; index < 0x50; index++) {
            record = DAT_001db78 + index * 0x64;
            map_object_finish(record);
        }
        DAT_0011fb64 = DAT_0011fb60;
        DAT_0011fac0 = 0;
        return;
    }

    for (index = 0; index < 0x50; index++) {
        record = DAT_001db78 + index * 0x64;
        map_object_update(record);
    }
    if (DAT_0011fb64 != -2) {
        DAT_0011fb64 = DAT_0011fb60;
    }
}
