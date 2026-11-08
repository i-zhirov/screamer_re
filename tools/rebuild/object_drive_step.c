/* The rebuilt `object_drive_step` module.
 *
 * The original sits at linear 0x1cee0 in STARTH.EXE and holds 83 instructions
 * and 297 bytes up to the first `ret`: the `edx` save, the frame, the 12-byte
 * local area, the drive-mode guard against `DAT_0000ffa4` and the state at
 * `+0x410`, the two force targets, the speed compare, the accelerate branch,
 * the brake branch, and the two clamps.
 *
 * The function controls the speed at `+0x418`.  While the mode `+0x410` is
 * above `DAT_0000ffa4`, the target is `(+0x2e4 + +0x2f0) * +0x3fc` and the
 * bound is `+0x2e4 * +0x3fc`.  Otherwise the target and the bound both equal
 * `+0x2e4 * +0x3fc`.  Below the target, the function clears the brake flag
 * `+0x2c0` and adds `+0x2e8` times the frame delta `DAT_00118e04`.  Above the
 * target, it sets the brake flag when the speed is above the bound and
 * subtracts `+0x2ec` times the frame delta.  Each branch clamps the speed to
 * the target.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_0000ffa4;
extern float DAT_00118e04;

void object_drive_step(char *object)
{
    float target;
    float bound;

    if (DAT_0000ffa4 < *(int *)(object + 0x410)) {
        target = (*(float *)(object + 0x2e4) + *(float *)(object + 0x2f0))
            * *(float *)(object + 0x3fc);
        bound = *(float *)(object + 0x2e4) * *(float *)(object + 0x3fc);
    } else {
        target = *(float *)(object + 0x2e4) * *(float *)(object + 0x3fc);
        bound = target;
    }
    if (*(float *)(object + 0x418) < target) {
        *(int *)(object + 0x2c0) = 0;
        *(float *)(object + 0x418) = *(float *)(object + 0x418)
            + *(float *)(object + 0x2e8) * DAT_00118e04;
        if (*(float *)(object + 0x418) > target) {
            *(float *)(object + 0x418) = target;
        }
    } else if (*(float *)(object + 0x418) > target) {
        if (*(float *)(object + 0x418) > bound) {
            *(int *)(object + 0x2c0) = 1;
        }
        *(float *)(object + 0x418) = *(float *)(object + 0x418)
            - *(float *)(object + 0x2ec) * DAT_00118e04;
        if (*(float *)(object + 0x418) < target) {
            *(float *)(object + 0x418) = target;
        }
    }
}
