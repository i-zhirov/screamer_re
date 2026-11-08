/* The rebuilt function at linear 0x2392c in STARTH.EXE.
 *
 * The plan row records a 344-byte body of 110 instructions up to the `ret 4`
 * at 0x23a81.  The next function starts at 0x23a94, so the store record keeps
 * the 360-byte extent and sets no separate function size.  The identity check
 * uses 110 instructions.
 *
 * The three direct callers at 0x23754, 0x237cf, and 0x2391e all sit in the
 * gauge panel at 0x2369c.  That panel is the callee of race_hud_draw at
 * 0x23420, which the frame update at 0x1ff70 calls at 0x2001a, so the record
 * takes the section Program functions.  The first two calls pass the point
 * records at object-3 offset 0x18f24 and 0x18f30 and the angle globals at
 * object-3 offset 0x100b8 and 0x100bc.  The third call passes a local point
 * and the angle 0x100bc, so the function draws a gauge marker.
 *
 * The function takes a point record in eax, a radius in edx, an angle bias in
 * ebx, and an angle in the stack argument.  The record holds two signed 32-bit
 * coordinates.  The function doubles the radius and both coordinates, because
 * the line drawer at 0x102e4 works in doubled screen coordinates.
 *
 * The angle delta is the argument minus the bias.  The function forms three
 * points on the circle of the doubled radius around the doubled centre:
 *
 *     point 1   delta
 *     point 2   delta + pi/2
 *     point 3   delta - pi/2
 *
 * Each point is (centre x + radius * sin(delta), centre y - radius * cos(delta)).
 * The function then draws one line from the centre to each point, so it draws
 * a three-arm marker that points forward and to both sides.
 *
 * The constants pi/2 and -pi arrive through globals, because the minimal image
 * does not load the constant segment.  The original calls sin at 0x4da72, cos
 * at 0x4da66, and the Watcom truncate helper at 0x4d9f4.  The harness links no
 * math library, so the local FPU stub supplies sin and cos and makes `__CHP`
 * truncate toward zero.  Each sum truncates after the centre term, because the
 * original calls the helper on the full double value.
 *
 * The three-arm marker, the gauge panel caller, and the two angle globals
 * support the name gauge_marker_draw.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern double sin(double value);
extern double cos(double value);

extern void gauge_line_draw(int x1, int y1, int x2, int y2);

extern float gauge_half_pi;
extern float gauge_minus_pi;

struct gauge_point {
    int x;
    int y;
};

void gauge_marker_draw(struct gauge_point *point, int radius, int angle_bias,
    float angle)
{
    int scale;
    int center_x;
    int center_y;
    float delta;
    int x1;
    int y1;
    int x2;
    int y2;
    int x3;
    int y3;

    scale = radius * 2;
    center_x = point->x * 2;
    center_y = point->y * 2;
    delta = angle - (float)angle_bias;

    x1 = (int)((double)center_x + (double)scale * sin((double)delta));
    y1 = (int)((double)center_y - (double)scale * cos((double)delta));

    delta = delta + gauge_half_pi;
    x2 = (int)((double)center_x + (double)scale * sin((double)delta));
    y2 = (int)((double)center_y - (double)scale * cos((double)delta));

    delta = delta + gauge_minus_pi;
    x3 = (int)((double)center_x + (double)scale * sin((double)delta));
    y3 = (int)((double)center_y - (double)scale * cos((double)delta));

    gauge_line_draw(center_x, center_y, x1, y1);
    gauge_line_draw(center_x, center_y, x2, y2);
    gauge_line_draw(center_x, center_y, x3, y3);
}
