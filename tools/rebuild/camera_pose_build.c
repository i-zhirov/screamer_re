/* The rebuilt `camera_pose_build` module.
 *
 * The original sits at linear 0x1a144 in STARTH.EXE.  The plan row counts the
 * fall-through branch up to the `ret` at 0x1a21e: 63 instructions and 219
 * bytes.  The function continues with the else branch at 0x1a21f to the `ret`
 * at 0x1a2b8, so the complete body holds 103 instructions and 373 bytes.  The
 * next function starts at 0x1a2bc, so the store record keeps the 376-byte
 * extent and sets no function size.  The identity check uses the plan count
 * 63.  The behavior check covers both branches.
 *
 * The only direct caller at 0x19ad6 is `player_update` at 0x19ab0.  It runs
 * this function as the fourth step on the player view object
 * `DAT_00110a20`, after `view_transition_update` at 0x19f00.  The function
 * writes the camera pose at `DAT_00109a0`: the three 11-bit angle indices at
 * `+0x00`, `+0x04`, and `+0x08`, and the three position values at `+0x0c`,
 * `+0x10`, and `+0x14`.  `camera_update` at 0x8ac78 reads this pose through
 * `basis_transform_build` at 0x8b610.
 *
 * The view object holds the base position at `+0x00`, `+0x04`, and `+0x08`,
 * the position delta at `+0x24`, `+0x28`, and `+0x2c`, the base angles at
 * `+0x30`, `+0x34`, and `+0x38`, the angle delta at `+0x3c`, `+0x40`, and
 * `+0x44`, and the transition flag `+0x58`.  `view_transition_update` forms
 * the two deltas and sets the flag.
 *
 * When the flag is set, the angle takes the base plus the delta.  When the
 * flag is clear, the angle takes the base alone.  The scale 1024.0 and the
 * divisor pi convert the angle to the table index, and the mask 0x7ff wraps
 * the index.  The position takes the base plus the delta when the flag is set,
 * and the base alone otherwise.
 *
 * The original calls the truncating helper at 0x4d9f4.  This harness links no
 * math library, so the local FPU stub makes `__CHP` truncate toward zero.  The
 * two constants arrive through globals, because the minimal image does not
 * load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
#define ANGLE_MASK 0x7ff

struct camera_pose {
    int angle[3];
    int position[3];
};

struct view_object {
    int position[3];
    int reserved_0c[3];
    int copy[3];
    int delta[3];
    float angle[3];
    float angle_delta[3];
    float saved_angle[3];
    int transition;
    int use_delta;
};

extern struct camera_pose camera_pose;
extern float view_angle_scale;
extern float view_angle_pi;

void camera_pose_build(struct view_object *view)
{
    if (view->use_delta) {
        camera_pose.angle[0] =
            (int)((view->angle[0] + view->angle_delta[0])
                * view_angle_scale / view_angle_pi) & ANGLE_MASK;
        camera_pose.angle[1] =
            (int)((view->angle[1] + view->angle_delta[1])
                * view_angle_scale / view_angle_pi) & ANGLE_MASK;
        camera_pose.angle[2] =
            (int)((view->angle[2] + view->angle_delta[2])
                * view_angle_scale / view_angle_pi) & ANGLE_MASK;
        camera_pose.position[0] = view->position[0] + view->delta[0];
        camera_pose.position[1] = view->position[1] + view->delta[1];
        camera_pose.position[2] = view->position[2] + view->delta[2];
    } else {
        camera_pose.angle[0] =
            (int)(view->angle[0] * view_angle_scale / view_angle_pi)
                & ANGLE_MASK;
        camera_pose.angle[1] =
            (int)(view->angle[1] * view_angle_scale / view_angle_pi)
                & ANGLE_MASK;
        camera_pose.angle[2] =
            (int)(view->angle[2] * view_angle_scale / view_angle_pi)
                & ANGLE_MASK;
        camera_pose.position[0] = view->position[0];
        camera_pose.position[1] = view->position[1];
        camera_pose.position[2] = view->position[2];
    }
}
