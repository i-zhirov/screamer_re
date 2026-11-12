/* The rebuilt `line_clip_draw` module.
 *
 * The original entry sits at linear 0x102e4 in STARTH.EXE.  The plan window
 * holds 16 instructions and 47 bytes up to the first `ret` at 0x10312, and the
 * identity check uses 16 instructions.  The branch at 0x1030b or 0x1030f enters
 * the clip body past that `ret`, and the last `ret` sits at 0x1055a.  The next
 * entry starts at 0x1057c, so the store record keeps the 664-byte extent and
 * sets no separate function size.
 *
 * The function takes the two line endpoints in `eax`, `edx`, `ebx`, and `ecx`
 * as x1, y1, x2, and y2.  It first sorts the endpoints so that x1 is not above
 * x2 and swaps the two y values with the x values.  It keeps the sorted
 * endpoints in the near-data cells 0x55c, 0x564, 0x560, and 0x568, clips the
 * line against the rectangle in 0x56c, 0x570, 0x574, and 0x578, and draws the
 * clipped line with the byte color at 0x55b into the video buffer at 0x21528.
 * The buffer row stride is 0x280, so the coordinates are doubled.
 *
 * The direct callers are `radar_world_draw` at 0x1057c and the three calls in
 * `gauge_marker_draw` at 0x2392c, and both read the same draw globals.  The
 * data flow supports the name `line_clip_draw`.  The original has no symbol, so
 * the record keeps the name with the confidence inferred.  Both callers sit in
 * the Program functions section, so the record takes the same section.
 *
 * The original has no EBP frame, saves every register through `pushal`, holds
 * no floating-point instruction, and makes no external call.  The two internal
 * `call 0x10547` sites are the Bresenham setup block of the same function.  The
 * module builds without `-of+` and needs no FPU stub and no assembly body.
 */
extern unsigned char DAT_0000055b;
extern int DAT_0000055c;
extern int DAT_00000560;
extern int DAT_00000564;
extern int DAT_00000568;
extern int DAT_0000056c;
extern int DAT_00000570;
extern int DAT_00000574;
extern int DAT_00000578;
extern unsigned char *DAT_00021528;

void line_clip_draw(int x1, int y1, int x2, int y2)
{
    int top;
    int bottom;
    int xmin;
    int xmax;
    int offset;
    int swap;
    int dx;
    int dy;
    int stride;
    int two;
    int error;
    int step;
    int count;
    unsigned char color;
    unsigned char *target;

    if (x1 > x2) {
        swap = x1;
        x1 = x2;
        x2 = swap;
        swap = y1;
        y1 = y2;
        y2 = swap;
    }
    DAT_0000055c = x1;
    DAT_00000564 = y1;
    DAT_00000560 = x2;
    DAT_00000568 = y2;

    top = DAT_00000574;
    if (y1 < top) {
        if (y2 >= top) {
            offset = (x1 - x2) * (top - y1) / (y1 - y2);
            x1 += offset;
            y1 = top;
            DAT_0000055c = x1;
            DAT_00000564 = y1;
        } else {
            return;
        }
    }
    if (y2 < top) {
        offset = (x2 - x1) * (top - y2) / (y2 - y1);
        x2 += offset;
        y2 = top;
        DAT_00000560 = x2;
        DAT_00000568 = y2;
    }

    bottom = DAT_00000578;
    if (y1 >= bottom) {
        if (y2 < bottom) {
            offset = (x1 - x2) * (bottom - y1) / (y1 - y2);
            x1 += offset;
            y1 = bottom;
            DAT_0000055c = x1;
            DAT_00000564 = y1;
        } else {
            return;
        }
    }
    if (y2 >= bottom) {
        offset = (x2 - x1) * (bottom - y2) / (y2 - y1);
        x2 += offset;
        y2 = bottom;
        DAT_00000560 = x2;
        DAT_00000568 = y2;
    }

    xmin = DAT_0000056c;
    if (x2 <= xmin) {
        return;
    }
    if (x1 < xmin) {
        offset = (y1 - y2) * (xmin - x1) / (x1 - x2);
        y1 += offset;
        x1 = xmin;
        DAT_00000564 = y1;
        DAT_0000055c = x1;
    }

    xmax = DAT_00000570;
    if (x1 >= xmax) {
        return;
    }
    if (x2 >= xmax) {
        offset = (y2 - y1) * (xmax - x2) / (x2 - x1);
        y2 += offset;
        x2 = xmax - 1;
        DAT_00000568 = y2;
        DAT_00000560 = x2;
    }

    target = DAT_00021528 + (y1 * 0x280 + x1);
    dx = x2 - x1;
    if (dx == 0) {
        dy = y2 - y1;
        if (dy == 0) {
            return;
        }
        stride = 0x280;
        goto compare;
    }
    dy = y2 - y1;
    stride = 0x280;

compare:
    if (dx < dy) {
        goto steep;
    }
    if (dy < 0) {
        dy = -dy;
        stride = -0x280;
        if (dx < dy) {
            goto steep;
        }
    }

    two = dy + dy;
    error = two - dx;
    step = dy - dx;
    step += step;
    color = DAT_0000055b;
    *target = color;
    count = dx;
    while (count != 0) {
        if (error > 0) {
            target += stride;
            error += step;
        } else {
            error += two;
        }
        *target = color;
        target++;
        *target = color;
        count--;
    }
    return;

steep:
    two = dx + dx;
    error = two - dy;
    step = dx - dy;
    step += step;
    color = DAT_0000055b;
    *target = color;
    count = dy;
    while (count != 0) {
        if (error > 0) {
            target += stride;
            error += step;
            *target = color;
            target++;
            *target = color;
        } else {
            error += two;
            target += stride;
            *target = color;
        }
        count--;
    }
}
