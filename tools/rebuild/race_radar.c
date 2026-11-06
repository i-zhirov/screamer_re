/* The rebuilt `race_radar` module.
 *
 * The original sits at linear 0x23a94 in STARTH.EXE and holds 208 instructions
 * and 870 bytes up to the `ret` at 0x23df9.  The record keeps the 872-byte
 * extent and sets no function size.
 *
 * The only caller at 0x2368c is the end of the HUD draw at 0x23420.  The frame
 * update at 0x1ff70 runs the HUD draw at 0x2001a after the view groups, so the
 * function draws a race HUD element.  The element is the 144x144 panel at
 * (8, 248): the function clears the panel through 0x25d28, renders the world
 * from the player position and heading through 0x1057c, and plots the nearby
 * records in the rotated frame.
 *
 * The zoom counter `DAT_0011d2a4` selects the scale: 0 returns, 1 uses 0x30,
 * 2 uses 0x60, and 3 uses 0x90.  The scale is the counter value times
 * 1/65536.  The input map at 0x2126d advances the counter from 0 through 3, so
 * the three values are the three radar ranges.
 *
 * The mode byte `DAT_0011d260` selects the record source.  Bit 0x10 scans the
 * 0x2a8-byte traffic records at `DAT_00113cd8` while the counter is below
 * `DAT_00115218`, and a record with the first field -1 is skipped.  Otherwise
 * bits 0 and 1 scan the 0x590-byte car records at `DAT_00110ab8` while the
 * record stays below `DAT_00110ab8 + DAT_0010f8ec * 0x590`.  Otherwise bit 4
 * plots the single record at `DAT_00118ee8` and `DAT_00118ef0` when
 * `DAT_0011d264` is set and the table entry at `DAT_00118e98` for the index
 * `DAT_00118ed4 ^ 1` is non-zero.
 *
 * A marker position is the rotated record delta times the scale around the
 * panel centre (39, 159), doubled and truncated.  The function stores the
 * position in `DAT_0011cb18` and `DAT_0011cb1c` and flushes the sprite list
 * through 0x25af0.  The final marker sits at (0x4c, 0x13c) with the mode 4.
 *
 * The original rounds the heading through 0x4d9f4, which truncates toward
 * zero, so the module uses a local truncating FPU stub.  The constants arrive
 * through globals, because the minimal image does not load the constant
 * segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_0011d2a4;
extern unsigned char DAT_0011d260;
extern int DAT_0011d264;
extern int DAT_00115218;
extern unsigned char DAT_00113cd8[];
extern int DAT_0010f8ec;
extern unsigned char DAT_00110ab8[];
extern int DAT_00118ed4;
extern int DAT_00118e98[];
extern int DAT_00118ee8;
extern int DAT_00118ef0;
extern int DAT_0011fdcc;
extern int DAT_0011fdd4;
extern float DAT_0011fde8;

extern unsigned char DAT_0000055b;
extern int DAT_0000056c;
extern int DAT_00000570;
extern int DAT_00000574;
extern int DAT_00000578;
extern int DAT_00115ce4;
extern int DAT_00115ce8;
extern int DAT_00115cec;
extern int DAT_00115cf0;
extern int DAT_0011cb18;
extern int DAT_0011cb1c;
extern int DAT_0011cb20;

extern float radar_angle_scale;
extern float radar_pi;
extern float radar_heading_scale;
extern float radar_center_x;
extern float radar_center_y;
extern float radar_scale;

extern void vector_rotate(int *out, int *in, float angle);
extern void map_fill(int x, int y, int width, int height);
extern void world_render(int x, int y, int angle);
extern void sprite_flush(void);

static void plot_marker(int dx, int dy, float scale, float heading)
{
    int delta[3];
    int rotated[3];

    delta[0] = dx;
    delta[2] = dy;
    vector_rotate(rotated, delta, heading);
    DAT_0011cb18 = (int)(
        (radar_center_x - (float)rotated[0] * scale) * radar_scale);
    DAT_0011cb1c = (int)(
        ((float)rotated[2] * scale + radar_center_y) * radar_scale);
    sprite_flush();
}

void race_radar(void)
{
    char *record;
    char *end;
    float scale;
    int angle;
    int heading;
    int index;

    if (DAT_0011d2a4 == 0) {
        return;
    } else if (DAT_0011d2a4 == 1) {
        angle = 0x30;
    } else if (DAT_0011d2a4 == 2) {
        angle = 0x60;
    } else if (DAT_0011d2a4 == 3) {
        angle = 0x90;
    } else {
        return;
    }

    DAT_0000055b = 0xff;
    DAT_0000056c = 8;
    DAT_00115ce4 = 8;
    DAT_00000574 = 0xf8;
    DAT_00115cec = 0xf8;
    DAT_00000570 = 0x98;
    DAT_00115ce8 = 0x98;
    DAT_00000578 = 0x188;
    DAT_00115cf0 = 0x188;
    map_fill(8, 0xf8, 0x90, 0x90);

    heading = (int)(DAT_0011fde8 * radar_angle_scale / radar_pi) & 0x7ff;
    world_render(DAT_0011fdcc, DAT_0011fdd4, heading);

    scale = (float)angle * radar_heading_scale;
    DAT_0011cb20 = 5;

    if (DAT_0011d260 & 0x10) {
        record = (char *)DAT_00113cd8;
        for (index = 0; index < DAT_00115218; index++) {
            if (*(int *)record >= 0) {
                plot_marker(
                    *(int *)(record + 0x3c) - DAT_0011fdcc,
                    *(int *)(record + 0x44) - DAT_0011fdd4,
                    scale,
                    DAT_0011fde8);
            }
            record += 0x2a8;
        }
    } else if (DAT_0011d260 & 3) {
        record = (char *)DAT_00110ab8;
        end = record + DAT_0010f8ec * 0x590;
        while (record < end) {
            plot_marker(
                *(int *)(record + 0x498) - DAT_0011fdcc,
                *(int *)(record + 0x4a0) - DAT_0011fdd4,
                scale,
                DAT_0011fde8);
            record += 0x590;
        }
    } else if (DAT_0011d260 & 4) {
        if (DAT_0011d264 != 0) {
            index = DAT_00118ed4 ^ 1;
            if (DAT_00118e98[index * 9] != 0) {
                plot_marker(
                    DAT_00118ee8 - DAT_0011fdcc,
                    DAT_00118ef0 - DAT_0011fdd4,
                    scale,
                    DAT_0011fde8);
            }
        }
    }

    DAT_0011cb18 = 0x4c;
    DAT_0011cb1c = 0x13c;
    DAT_0011cb20 = 4;
    sprite_flush();
    DAT_00115ce8 = 0x280;
    DAT_00115ce4 = 0;
    DAT_00115cec = 0;
    DAT_00115cf0 = 0x190;
}
