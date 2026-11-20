/* The rebuilt `race_gauges` module.
 *
 * The original sits at linear 0x2369c in STARTH.EXE.  The dispatch head holds
 * 32 instructions and 96 bytes up to the default `ret` at 0x236fb.  The next
 * record starts at 0x2392c, so the store record keeps the 656-byte extent and
 * sets no separate function size.  The identity check uses 32 instructions.
 * The two case blocks after the head hold the rest of the body, so this module
 * rebuilds the full function.
 *
 * The only direct caller is race_hud_draw at 0x234c5, which calls the function
 * as the gauge panel after the five view groups.  The panel blits a cockpit
 * image into the video buffer and then selects one of two gauge layouts from
 * the mode dword at object-3 offset 0x100c0:
 *
 *     mode 1   the COCK image at 0x18f0c, the two gauge angles filtered from
 *              the phase 0xfe28 and the speed 0x100c4, the two markers at
 *              0x18f24 and 0x18f30, and the active object list
 *     mode 2   the cdfront image at 0x18f10, the two counter texts, the right
 *              gauge angle filtered from the speed, and one marker at a local
 *              point
 *     other    no case block runs
 *
 * The blit helper at 0x102b0 copies a sequence of positive spans and moves the
 * destination on a negative span.  The filter helper at 0x22b80 applies the
 * one-pole filter to a float cell.  The marker at 0x2392c draws a three-arm
 * gauge marker.  The list helper at 0x25af0 draws the active objects.
 *
 * The constants 4.084070563316345, 3.612831652164459, 0.017, the float pi, and
 * the double pi arrive through globals, because the minimal image does not load
 * the constant segment.  The original calls the truncate helper at 0x4d9f4
 * for the integer conversion, so the module uses a truncating `__CHP`
 * stub.
 *
 * The gauge panel caller, the two gauge markers, and the cockpit blits support
 * the name race_gauges.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
struct gauge_point {
    int x;
    int y;
};

extern void *DAT_0018f18;
extern void *DAT_0018f0c;
extern void *DAT_0018f10;
extern int DAT_0018f1c;
extern int DAT_0018f20;
extern struct gauge_point DAT_0018f24;
extern struct gauge_point DAT_0018f30;
extern int DAT_0018f3c[];

extern float DAT_0000fd9c;
extern float DAT_0000fda0;
extern float DAT_0000fe28;
extern float DAT_00100b8;
extern float DAT_00100bc;
extern int DAT_00100c0;
extern float DAT_00100c4;
extern int DAT_00100c8;

extern unsigned char DAT_0000055b;
extern int DAT_0000056c;
extern int DAT_00000570;
extern int DAT_00000574;
extern int DAT_00000578;

extern int DAT_001cb18;
extern int DAT_001cb1c;
extern int DAT_001cb20;
extern void *DAT_0021528;

extern char DAT_00000e02[];
extern char DAT_00000e08[];

extern double gauge_scale_front;
extern double gauge_scale_side;
extern double gauge_tick;
extern float gauge_pi;
extern double gauge_pi_max;

extern void data_blit(void *source, void *destination);
extern void filter_update(float *value, float target, int index);
extern void gauge_marker_draw(struct gauge_point *point, int radius,
    int angle_bias, float angle);
extern void draw_objects(void);
extern void text_draw(int x, int y, char *text, int font, int color);
extern int sprintf(char *buffer, const char *format, ...);

void race_gauges(void)
{
    char text[0x100];
    struct gauge_point point;
    double value;
    int count;

    DAT_0000056c = 0;
    DAT_00000570 = 0x280;
    DAT_00000574 = 0;
    DAT_00000578 = 0x190;
    data_blit(DAT_0018f18, DAT_0021528);

    if (DAT_00100c0 == 1) {
        data_blit(DAT_0018f0c, DAT_0021528);
        DAT_0000055b = 0xff;
        value = ((double)DAT_0000fe28 / DAT_0000fd9c) * gauge_scale_front;
        filter_update(&DAT_00100b8, (float)value, 0x5a);
        gauge_marker_draw(&DAT_0018f24, DAT_0018f1c, 2, DAT_00100b8);
        value = ((double)DAT_00100c4 / DAT_0000fda0) * gauge_scale_side;
        if (value > gauge_scale_side) {
            value = gauge_scale_side;
        }
        filter_update(&DAT_00100bc, (float)value, 0x5a);
        point.x = 0xcf;
        point.y = 0xb0;
        gauge_marker_draw(&DAT_0018f30, DAT_0018f1c, 2, DAT_00100bc);
        DAT_001cb1c = DAT_0018f3c[DAT_00100c8 * 3 + 1];
        DAT_001cb18 = DAT_0018f3c[DAT_00100c8 * 3];
        DAT_001cb20 = DAT_00100c8 + DAT_0018f20;
        draw_objects();
    } else if (DAT_00100c0 == 2) {
        data_blit(DAT_0018f10, DAT_0021528);
        DAT_0000055b = 0x75;
        count = (int)((double)DAT_0000fe28 * gauge_tick);
        sprintf(text, DAT_00000e02, count);
        text_draw(0x21e, 0x176, text, 2, 7);
        sprintf(text, DAT_00000e08, DAT_00100c8);
        text_draw(0x260, 0x176, text, 2, 1);
        value = ((double)DAT_00100c4 / DAT_0000fda0) * gauge_pi;
        if (value > gauge_pi_max) {
            value = gauge_pi_max;
        }
        filter_update(&DAT_00100bc, (float)value, 0x5a);
        point.x = 0x10f;
        point.y = 0xa8;
        gauge_marker_draw(&point, 0x1e, 2, DAT_00100bc);
    }
}
