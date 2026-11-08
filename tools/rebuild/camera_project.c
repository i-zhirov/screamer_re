/* The rebuilt `camera_project` module.
 *
 * The original sits at linear 0x82c14 in STARTH.EXE and holds 72 instructions
 * and 277 bytes up to the `ret` at 0x82d28.  The next recorded function starts
 * at 0x82e0c, so the store record keeps the 504-byte extent and sets no
 * separate function size.  The identity check uses 72 instructions.
 *
 * The only direct caller is `camera_update` at 0x8ac99.  That helper runs in
 * the race frame path, so the record takes the section `race_frame callees`.
 *
 * The function reads the camera record from the global `camera_state` at
 * 0x7a7ac.  The record holds three angle indices at `+0x00`, `+0x04`, and
 * `+0x08`.  An angle index selects an 8-byte table entry at 0x203d0.  The
 * entry holds a signed 16-bit cosine in the low word at `+0x00` and a signed
 * 16-bit sine in the low word at `+0x04`.  The function shifts each value one
 * bit to the right with the arithmetic shift, which converts the Q15 value to
 * Q14.
 *
 * The first pass stores the three cosine and sine pairs in the globals
 * 0x724cc, 0x724d0, 0x724d4, 0x724d8, 0x724dc, and 0x724e0.  The second pass
 * forms the complement index `0x7ff - angle` for each angle and stores the
 * second three pairs in the globals 0x724e4, 0x724e8, 0x724ec, 0x724f0,
 * 0x724f4, and 0x724f8.  The function reads the camera pointer and the three
 * angles again for the second pass.
 *
 * The original saves all registers and the four segment registers.  It has no
 * EBP frame and uses no floating-point instruction.  The module builds without
 * `-of+` and links no FPU stub.
 */
#define MIRROR_BASE 0x7ff
#define FIXED_SHIFT 1

struct camera {
    int angle0;
    int angle1;
    int angle2;
};

struct projection_angle {
    short cosine;
    short reserved_00;
    short sine;
    short reserved_04;
};

extern struct camera *camera_state;
extern struct projection_angle projection_angles[];
extern int projection_cos_0;
extern int projection_sin_0;
extern int projection_cos_1;
extern int projection_sin_1;
extern int projection_cos_2;
extern int projection_sin_2;
extern int projection_complement_cos_0;
extern int projection_complement_sin_0;
extern int projection_complement_cos_1;
extern int projection_complement_sin_1;
extern int projection_complement_cos_2;
extern int projection_complement_sin_2;

void camera_project(void)
{
    struct camera *camera;
    int angle0;
    int angle1;
    int angle2;

    camera = camera_state;
    angle0 = camera->angle0;
    angle1 = camera->angle1;
    angle2 = camera->angle2;

    projection_cos_0 = projection_angles[angle0].cosine >> FIXED_SHIFT;
    projection_sin_0 = projection_angles[angle0].sine >> FIXED_SHIFT;
    projection_cos_1 = projection_angles[angle1].cosine >> FIXED_SHIFT;
    projection_sin_1 = projection_angles[angle1].sine >> FIXED_SHIFT;
    projection_cos_2 = projection_angles[angle2].cosine >> FIXED_SHIFT;
    projection_sin_2 = projection_angles[angle2].sine >> FIXED_SHIFT;

    camera = camera_state;
    angle0 = MIRROR_BASE - camera->angle0;
    angle1 = MIRROR_BASE - camera->angle1;
    angle2 = MIRROR_BASE - camera->angle2;

    projection_complement_cos_0 =
        projection_angles[angle0].cosine >> FIXED_SHIFT;
    projection_complement_sin_0 =
        projection_angles[angle0].sine >> FIXED_SHIFT;
    projection_complement_cos_1 =
        projection_angles[angle1].cosine >> FIXED_SHIFT;
    projection_complement_sin_1 =
        projection_angles[angle1].sine >> FIXED_SHIFT;
    projection_complement_cos_2 =
        projection_angles[angle2].cosine >> FIXED_SHIFT;
    projection_complement_sin_2 =
        projection_angles[angle2].sine >> FIXED_SHIFT;
}
