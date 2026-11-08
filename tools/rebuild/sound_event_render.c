/* The rebuilt `sound_event_render` module.
 *
 * The original sits at linear 0x836ac in STARTH.EXE.  The plan row records a
 * 351-byte body of 100 instructions.  The 100th instruction is the `ret` at
 * 0x8380a, so the identity check uses 100 instructions.  The store record keeps
 * the 1532-byte extent, which reaches the next record at 0x83ca8, and it sets
 * no separate function size.
 *
 * The only caller is `sound_effects_update` at 0x81ab0.  It calls this
 * function as the event render inside the per-frame event path.  The caller
 * sits in the object_update_all group, so the record takes the section
 * object_update_all callees.  The caller passes no argument, so the function
 * reads the global view pointer 0x74460.
 *
 * The function reads the view record P from the global 0x74460.  It forms the
 * three axis values from +0x13c, +0x140, and 0x80 - +0x144, takes the square
 * root of the sum of the squares, and stores the rounded distance at +0x1f0.
 *
 * The function then adds the signed short offsets at Q + 4, Q + 6, and Q + 8
 * to the body position at +0x16c, +0x170, and +0x174, where Q is the pointer at
 * P + 0.  A set bit 0x2000 of the flag 0x74530 calls the prepare helper at
 * 0xc2270.
 *
 * The function scales each coordinate by 0x13333 / 0x10000 with an arithmetic
 * shift.  A signed 32-bit product overflow keeps all three original values,
 * because the original reloads the saved triple.  The function then calls the
 * frame selector at 0x8a848 with the three scaled values.  The selector sets
 * the vertex list and the output buffer that the render loop uses.
 *
 * The selector call is the second of the two calls in the plan row.  The three
 * tail jumps at 0x837f6, 0x837c2, and 0x837d2 leave the 100-instruction
 * window.  The far tail at 0x83863 patches the coordinate immediates of its own
 * code with the scaled values, and the near and clipped tails at 0x83880 and
 * 0x83b48 patch other blocks.  The module records the selected tail in the
 * global `render_path` instead of a call into code outside the window.
 *
 * The original has no EBP frame.  The module takes the square root from the
 * local `sound_event_render_fpu.asm` stub, because the harness links no math
 * library, and it adds 0.5 before the truncating cast to match the
 * round-to-nearest `fist` of the original.
 */
#define SCALE_NUM 0x13333
#define SCALE_SHIFT 16
#define SCALE_MAX 27306
#define SCALE_MIN (-27306)
#define PREPARE_FLAG 0x2000

#define TAIL_NEAR 1
#define TAIL_CLIPPED 2
#define TAIL_FAR 3

extern double sqrt(double value);

extern void *render_view;
extern int render_flags;
extern int render_depth_limit;

extern int render_path;

extern void render_prepare(void);
extern void render_frame_select(int x, int y, int z);

void sound_event_render(void)
{
    char *view;
    char *root;
    int axis_x;
    int axis_y;
    int depth;
    int axis_z;
    double square_sum;
    int body_x;
    int body_y;
    int body_z;
    int scale_x;
    int scale_y;
    int scale_z;
    int bias;

    view = (char *)render_view;
    axis_x = *(int *)(view + 0x13c);
    axis_y = *(int *)(view + 0x140);
    depth = *(int *)(view + 0x144);
    axis_z = 0x80 - depth;
    square_sum = (double)axis_x * axis_x + (double)axis_y * axis_y
        + (double)axis_z * axis_z;
    *(int *)(view + 0x1f0) = (int)(sqrt(square_sum) + 0.5);

    root = *(char **)view;
    body_x = *(int *)(view + 0x16c);
    body_y = *(int *)(view + 0x170);
    body_z = *(int *)(view + 0x174);
    body_x += *(short *)(root + 4);
    body_y += *(short *)(root + 6);
    body_z += *(short *)(root + 8);

    if ((render_flags & PREPARE_FLAG) != 0) {
        render_prepare();
    }

    if (body_x > SCALE_MAX || body_x < SCALE_MIN
        || body_y > SCALE_MAX || body_y < SCALE_MIN
        || body_z > SCALE_MAX || body_z < SCALE_MIN) {
        scale_x = body_x;
        scale_y = body_y;
        scale_z = body_z;
    } else {
        scale_x = (body_x * SCALE_NUM) >> SCALE_SHIFT;
        scale_y = (body_y * SCALE_NUM) >> SCALE_SHIFT;
        scale_z = (body_z * SCALE_NUM) >> SCALE_SHIFT;
    }

    render_frame_select(scale_x, scale_y, scale_z);

    bias = *(int *)root;
    if (depth + bias >= -8) {
        render_path = TAIL_NEAR;
        return;
    }
    if (depth - bias <= render_depth_limit) {
        render_path = TAIL_CLIPPED;
        return;
    }
    render_path = TAIL_FAR;
}
