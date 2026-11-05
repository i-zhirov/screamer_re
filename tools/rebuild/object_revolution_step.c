/* The rebuilt `object_revolution_step` module.
 *
 * The original sits at linear 0x1d30c in STARTH.EXE and holds 151 instructions
 * and 589 bytes: the six register saves, the frame, the 8-byte local area, the
 * revolution accumulation loop, the fraction wrap at 1.0, the gear toggle, the
 * two table index updates, the two truncations, the position shift, and the
 * angle call at 0x226bc.  The record keeps the 592-byte extent.
 *
 * The function accumulates the revolution fraction at `+0x250`.  Each step
 * divides the speed at `+0x418` times the frame delta `DAT_00118e04` by the
 * axis length from 0x222e0.  When the fraction reaches 1.0, the function clears
 * it, toggles the gear at `+0x254`, and advances the table index at `+0x24c`
 * from the entry at `+0x10`.  It then builds the matrix from 0x1d55c and stores
 * the two truncated positions at `+0x258` and `+0x260`.
 *
 * When the fraction stays below 1.0, the function builds the matrix once, sets
 * the output at `+0x25c` from the entry at `+0xc`, shifts the three position
 * pairs, and stores the angle from 0x226bc at `+0x27c`.
 *
 * The callees stay external.  The harness provides the axis, the matrix, the
 * square root, and the angle stubs.  The two conversions to an integer use the
 * Watcom helper at 0x4d9f4, which truncates toward zero.  The module uses a
 * module-local FPU stub, because the shared stub rounds to nearest.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern float DAT_00118e04;
extern unsigned int DAT_0010f8e4;

extern void object_revolution_matrix(char *object);
extern float object_revolution_axis(float *matrix, float fraction);
extern double object_revolution_length(double value);
extern float object_revolution_angle(float x, float y, float z);

void object_revolution_step(char *object)
{
    float delta;
    float dx;
    float dy;

    for (;;) {
        delta = *(float *)(object + 0x418) * DAT_00118e04;
        delta = delta / object_revolution_axis(
            (float *)(object + 0x284), *(float *)(object + 0x250));
        *(float *)(object + 0x250) = *(float *)(object + 0x250) + delta;
        if (*(float *)(object + 0x250) < 1.0f) {
            break;
        }
        *(float *)(object + 0x250) = 0;
        if (*(int *)(object + 0x254) == 0) {
            *(int *)(object + 0x254) = 1;
        } else {
            *(int *)(object + 0x254) = 0;
            *(int *)(object + 0x24c) = *(int *)((char *)(*(int *)((char *)
                DAT_0010f8e4 + *(int *)(object + 0x248) * 36))
                + *(int *)(object + 0x24c) * 56 + 0x10);
        }
        object_revolution_matrix(object);
        dx = *(float *)(object + 0x2a8) - (float)*(int *)(object + 0x258);
        dy = *(float *)(object + 0x2ac) - (float)*(int *)(object + 0x260);
        delta = delta - (float)object_revolution_length(
            (double)(dx * dx + dy * dy));
        *(int *)(object + 0x258) = (int)*(float *)(object + 0x2a8);
        *(int *)(object + 0x260) = (int)*(float *)(object + 0x2ac);
    }
    object_revolution_matrix(object);
    *(int *)(object + 0x258) = (int)*(float *)(object + 0x2a8);
    *(int *)(object + 0x260) = (int)*(float *)(object + 0x2ac);
    *(int *)(object + 0x25c) = *(int *)((char *)(*(int *)((char *)
        DAT_0010f8e4 + *(int *)(object + 0x248) * 36))
        + *(int *)(object + 0x24c) * 56 + 0xc);
    *(float *)(object + 0x270) = *(float *)(object + 0x264);
    *(float *)(object + 0x278) = *(float *)(object + 0x26c);
    *(float *)(object + 0x264) = *(float *)(object + 0x2b4);
    *(float *)(object + 0x26c) = *(float *)(object + 0x2b8);
    *(float *)(object + 0x280) = *(float *)(object + 0x27c);
    *(float *)(object + 0x27c) = object_revolution_angle(
        *(float *)(object + 0x264), *(float *)(object + 0x268),
        *(float *)(object + 0x26c));
}
