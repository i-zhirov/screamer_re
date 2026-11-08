/* The rebuilt `opponent_path_update` module.
 *
 * The original sits at linear 0x20b1c in STARTH.EXE and holds 166 instructions
 * and 516 bytes up to the `ret 4` at 0x20d1d.  The next function starts at
 * 0x20d20, so the record keeps the 516-byte extent and sets no function size.
 *
 * The only direct caller at 0x20a12 is `opponent_update` at 0x20954.  It passes
 * the state base `DAT_0010f174`, the path record at `+0x44`, and the frame step
 * from the record float `+0xc` scaled by `DAT_00118e04`.  The caller is in the
 * object_update_all group, so the module takes the section object_update_all
 * callees.
 *
 * The path record holds the track index at `+0`, the node index at `+4`, the
 * segment phase at `+8`, and the direction flag at `+0xc`.  The callee at
 * 0x222e0 returns the curve speed from the control block at `+0x3c` and the
 * phase.  The phase advances by the frame step divided by the speed.  When the
 * phase reaches one, the function clears the phase.  A clear direction flag
 * sets the flag and keeps the node.  A set direction flag clears the flag and
 * moves the node index through the next field at `+0x10` of the 56-byte node.
 *
 * The callee at 0x22378 evaluates the spline segment and writes the target
 * position and angle at `+0x60`, `+0x64`, and `+0x68`, and the next control
 * values at `+0x6c` and `+0x70`.  The function subtracts the distance from the
 * integer position `+0x10` and `+0x18` to the target from the frame step, and
 * it truncates the target into `+0x10`, `+0x14`, and `+0x18`.
 *
 * On the segment path the function copies the history values, takes the arc
 * sine from 0xc5e42 of the new lateral value at `+0x1c`, and forms the heading
 * at `+0x34` with pi/2 at object-3 offset 0xce1 or 2.5 pi at 0xce5.  It then
 * wraps the heading into one turn with 2 pi at 0xce9 and -2 pi at 0xced.
 *
 * The arc sine, the square root, the absolute value, and the truncation come
 * from the local FPU assembly file, because the harness links no math library
 * and the shared stub leaves round-to-nearest.  The original has the EBP frame,
 * so the module builds with `-of+`.
 */
extern float DAT_00100ce1;
extern float DAT_00100ce5;
extern float DAT_00100ce9;
extern float DAT_00100ced;

extern float opponent_curve_speed(void *curve, float phase);
extern void opponent_curve_eval(unsigned int state, void *path);
extern double sqrt(double value);
extern double asin(double value);
extern double fabs(double value);

void opponent_path_update(unsigned int state, void *body, float delta)
{
    char *path = (char *)body;
    char *entry;
    char *node;
    float phase;
    float speed;
    float dx;
    float dy;
    float angle;

    for (;;) {
        speed = opponent_curve_speed(path + 0x3c, *(float *)(path + 8));
        phase = *(float *)(path + 8) + delta / speed;
        *(float *)(path + 8) = phase;
        if (1.0f > phase) {
            break;
        }
        *(float *)(path + 8) = 0;
        if (*(int *)(path + 0xc) == 0) {
            *(int *)(path + 0xc) = 1;
        } else {
            *(int *)(path + 0xc) = 0;
            entry = (char *)(state
                + (unsigned int)(*(int *)path * 36));
            node = *(char **)entry + *(int *)(path + 4) * 56;
            *(int *)(path + 4) = *(int *)(node + 0x10);
        }
        opponent_curve_eval(state, path);
        dx = *(float *)(path + 0x60) - (float)*(int *)(path + 0x10);
        dy = *(float *)(path + 0x64) - (float)*(int *)(path + 0x18);
        delta = (float)((double)delta
            - sqrt((double)(dx * dx + dy * dy)));
        *(int *)(path + 0x10) = (int)*(float *)(path + 0x60);
        *(int *)(path + 0x18) = (int)*(float *)(path + 0x64);
    }
    opponent_curve_eval(state, path);
    *(int *)(path + 0x10) = (int)*(float *)(path + 0x60);
    *(int *)(path + 0x14) = (int)*(float *)(path + 0x68);
    *(int *)(path + 0x18) = (int)*(float *)(path + 0x64);
    *(float *)(path + 0x28) = *(float *)(path + 0x1c);
    *(float *)(path + 0x30) = *(float *)(path + 0x24);
    *(float *)(path + 0x1c) = *(float *)(path + 0x6c);
    *(float *)(path + 0x24) = *(float *)(path + 0x70);
    *(float *)(path + 0x38) = *(float *)(path + 0x34);
    angle = (float)fabs(asin((double)*(float *)(path + 0x1c)));
    if (*(float *)(path + 0x24) > 0.0f) {
        angle = angle + DAT_00100ce1;
    } else {
        angle = DAT_00100ce5 - angle;
    }
    *(float *)(path + 0x34) = angle;
    while (*(float *)(path + 0x34) < 0.0f) {
        *(float *)(path + 0x34) = *(float *)(path + 0x34)
            + DAT_00100ce9;
    }
    while (*(float *)(path + 0x34) >= DAT_00100ce9) {
        *(float *)(path + 0x34) = *(float *)(path + 0x34)
            + DAT_00100ced;
    }
}
