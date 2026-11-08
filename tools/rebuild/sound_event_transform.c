/* The rebuilt `sound_event_transform` module.
 *
 * The original sits at linear 0x835c0 in STARTH.EXE.  The plan row records a
 * 225-byte body of 56 instructions.  The body ends at the `ret` at 0x836a0, and
 * the next store record starts at 0x836ac, so the record keeps the 236-byte
 * extent and sets no separate function size.  The identity check uses 56
 * instructions.
 *
 * The only caller is `sound_effects_update` at 0x81ab0.  It calls this function
 * in the per-frame event path, after the event helper at 0x8a8d8, so the record
 * takes the section object_update_all callees.  The function reads no argument.
 *
 * The helper at 0x8a8d8 sets the source pointer at 0x7446c from the event
 * record.  The setup at 0x83560 patches the twelve immediate matrix fields of
 * this function from the view basis at view + 0x118.  The on-disk value
 * 0xaaaabbbb is the linker fill value, so the original patches its own code.
 * The rebuild reads the twelve values from the global `event_matrix` instead.
 *
 * The function walks the 6-byte source records at the source pointer until the
 * signed short x value is 0x7fff.  Each record holds the signed short x, y, and
 * z values.
 *
 * The function forms the camera-space point from the 3x3 basis and the three
 * translations with the Q14 arithmetic shift:
 *
 *     out0 = ((m0 * x + m1 * y + m2 * z) >> 14) + t0
 *     out1 = ((m3 * x + m4 * y + m5 * z) >> 14) + t1
 *     out2 = ((m6 * x + m7 * y + m8 * z) >> 14) + t2
 *
 * It stores the three values in the point list at 0x4f308, one 12-byte entry
 * per source record.
 *
 * The third value selects the packed screen coordinate.  A positive value stores
 * the clipped point 0x10001000.  Otherwise the projection table at 0x3badc
 * scales the first two values, and the function stores the packed 16-bit pair at
 * the ES destination from the selector at 0x2149c, one dword per 6-byte stride.
 * The rebuild models the destination with the `event_screen` pointer, because
 * the flat harness keeps one data segment.
 *
 * The function uses no floating-point instruction and has no EBP frame, so the
 * module builds without `-of+` and links no FPU stub.
 */
#define BASIS_SHIFT 14
#define CLIPPED_POINT 0x10001000
#define SCREEN_X_BIAS 0x13f0000

extern short *event_source;
extern int *event_points;
extern char *event_screen;
extern int *event_projection;
extern int DAT_0007a7dc;
extern int event_matrix[12];

void sound_event_transform(void)
{
    int offset;
    int x;
    int y;
    int z;
    int out0;
    int out1;
    int out2;
    int scale;
    int prod0;
    int prod1;
    int packed;

    offset = 0;
    for (;;) {
        short *record;

        record = (short *)((char *)event_source + offset);
        x = record[0];
        if (x == 0x7fff) {
            break;
        }
        y = record[1];
        z = record[2];

        out0 = ((event_matrix[0] * x + event_matrix[1] * y
            + event_matrix[2] * z) >> BASIS_SHIFT) + event_matrix[9];
        out1 = ((event_matrix[3] * x + event_matrix[4] * y
            + event_matrix[5] * z) >> BASIS_SHIFT) + event_matrix[10];
        out2 = ((event_matrix[6] * x + event_matrix[7] * y
            + event_matrix[8] * z) >> BASIS_SHIFT) + event_matrix[11];

        *(int *)((char *)event_points + offset * 2) = out0;
        *(int *)((char *)event_points + offset * 2 + 4) = out1;
        *(int *)((char *)event_points + offset * 2 + 8) = out2;

        if (out2 > 0) {
            packed = CLIPPED_POINT;
        } else {
            scale = event_projection[out2];
            prod0 = out0 * scale;
            prod1 = out1 * scale;
            prod1 >>= 16;
            prod0 += prod0;
            prod0 += SCREEN_X_BIAS;
            prod1 += prod1;
            prod1 += DAT_0007a7dc;
            packed = (prod0 & 0xffff0000) | (prod1 & 0xffff);
        }
        *(int *)(event_screen + offset) = packed;

        offset += 6;
    }
}
