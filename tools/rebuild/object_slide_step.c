/* The rebuilt `object_slide_step` module.
 *
 * The original sits at linear 0x1df34 in STARTH.EXE and holds 65 instructions
 * and 215 bytes: the `edx` save, the frame, the 8-byte local area, the guard on
 * the low bit of the state at `+0x25c`, the slide from `+0x418`, `+0x2f4`,
 * `+0x27c`, and `+0x280`, the clamp between -2.0 and 2.0, the three
 * `filter_update` calls at `0x22b80`, and the epilogue.
 *
 * The four constants arrive through the globals `slide_max`, `slide_min`,
 * `slide_half`, and `slide_rate`.  The harness sets the exact bit patterns.
 * This keeps the float math out of the constant segment, which the shared
 * harness image does not load for this module.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern void filter_update(float *p, float f, int i);
extern double fabs(double value);
extern float slide_max;
extern float slide_min;
extern double slide_half;
extern double slide_rate;

void object_slide_step(void *object)
{
    float slide;

    if ((*(unsigned char *)((char *)object + 0x25c) & 1) != 0) {
        slide = *(float *)((char *)object + 0x418)
            * *(float *)((char *)object + 0x2f4);
        slide = slide * slide;
        slide = slide * (*(float *)((char *)object + 0x27c)
            - *(float *)((char *)object + 0x280));
    } else {
        slide = 0.0f;
    }
    if (slide > slide_max) {
        slide = slide_max;
    } else if (slide < slide_min) {
        slide = slide_min;
    }
    filter_update((float *)((char *)object + 0x4c8), slide * slide_half, 0x4f);
    filter_update((float *)((char *)object + 0x4b8), slide * slide_rate, 0x4c);
    filter_update((float *)((char *)object + 0x4b0),
        -fabs(slide * slide_rate), 0x4c);
}
