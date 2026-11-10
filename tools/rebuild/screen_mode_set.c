/* The rebuilt `screen_mode_set` module.
 *
 * The original sits at linear 0x4c850 in STARTH.EXE and holds 58 instructions
 * and 159 bytes up to the `ret` at 0x4c8ee.  The next recorded function starts
 * at 0x4c8f0, so the store record keeps the 160-byte extent and sets no
 * separate function size.  The identity check uses 58 instructions.
 *
 * The function takes the width in `eax`, the height in `edx`, and the third
 * value in `ebx`, and it writes the geometry fields of the record at the
 * global pointer `PTR_DAT_0008a844`.  The fields start at `+0x14`.  The color
 * field at `+0x10` belongs to `race_data_load` at 0x131a8.
 *
 * The five direct callers pass the mode 0x140 by 0x190 with the value 0x104 or
 * a computed height:
 *
 *     race_data_load       0x131a8
 *     player_view_setup    0x19ae8
 *     view_transition_update 0x19f00
 *     quit_confirm_screen  0x219ec
 *     menu_space_init      0x29d08
 *
 * The callers name the function `screen_mode_set`, so the record keeps that
 * name with the confidence inferred.  The original has no symbol.
 *
 * The function forms the doubled width and the incremented height, stores the
 * signed half values and the 16.16 fixed-point values, and calls the selector
 * setup at 0x8bc7c.  The 16.16 fields hold `value << 16`.  The half fields use
 * the arithmetic shift, so a negative input rounds toward negative infinity.
 * The field `+0x2c` holds the doubled width divided by two with the signed
 * division, so a negative input truncates toward zero.
 *
 * The original has the EBP frame and spills the three register arguments to
 * the frame, so the module builds with `-of+`.  The function uses no
 * floating-point instruction, so the module needs no FPU stub.
 */
struct screen_mode_info {
    unsigned char pad_00[0x14];
    int field_14;
    int field_18;
    int field_1c;
    int field_20;
    int field_24;
    int field_28;
    int field_2c;
    int field_30;
    int field_34;
    int field_38;
    int field_3c;
    int field_40;
    int field_44;
    int field_48;
};

extern struct screen_mode_info *PTR_DAT_0008a844;

extern void video_selector_setup(void);

void screen_mode_set(int width, int height, int value)
{
    struct screen_mode_info *info;
    int doubled_width;
    int next_height;

    info = PTR_DAT_0008a844;
    doubled_width = width * 2;
    next_height = height + 1;
    info->field_44 = doubled_width;
    info->field_48 = next_height;
    info->field_14 = doubled_width - 1;
    info->field_18 = next_height - 1;
    info->field_1c = info->field_14 >> 1;
    info->field_20 = info->field_18 >> 1;
    info->field_24 = info->field_14 << 16;
    info->field_28 = info->field_18 << 16;
    info->field_2c = info->field_44 / 2;
    info->field_30 = value;
    info->field_34 = info->field_2c >> 1;
    info->field_38 = value >> 1;
    info->field_3c = info->field_2c << 16;
    info->field_40 = value << 16;
    video_selector_setup();
}
