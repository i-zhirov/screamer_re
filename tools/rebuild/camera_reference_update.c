/* The rebuilt `camera_reference_update` module.
 *
 * The original sits at linear 0x212d0 in STARTH.EXE.  The plan row counts 63
 * bytes and 20 instructions to the fall-through `ret` at 0x2130e.  The else
 * branch continues from 0x2130f to the `ret` at 0x21335, so the complete body
 * holds 102 bytes.  The next function `options_screen` starts at 0x21338 after
 * the two padding bytes at 0x21336, so the store record keeps the 104-byte
 * extent and sets no separate function size.  The identity check uses the plan
 * count 20.
 *
 * The only direct caller at 0x19ae0 is `player_update` at 0x19ab0.  It runs
 * this function as its fifth step on the player view object `DAT_00110a20`,
 * after `camera_pose_build` at 0x1a144, so the record takes the section
 * `player_update callees`.
 *
 * The function publishes the shared camera reference.  The two cells
 * `DAT_0013ccc` and `DAT_0013cd0` hold the camera world position, and the cell
 * `DAT_0013cd4` holds the camera heading.  `sound_emitter_update` at 0x21b9c
 * reads the two position cells and the heading, and `opponent_update` at
 * 0x20954 reads the two position cells as the reference point for the
 * proximity test.
 *
 * The control byte `DAT_00118e1d` selects the source.  When bit 0 is set and
 * bit 0x20 is clear, the function copies the player world position
 * `DAT_0011fdcc` and `DAT_0011fdd4` and the player heading `DAT_0011fde8`.
 * Otherwise it copies the view saved copies `+0x0c` and `+0x14` and the view
 * yaw `+0x34`.  The player view object holds `+0x0c` and `+0x14` as the saved
 * x and z values and `+0x34` as the yaw.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * original holds the two floating-point instructions `fld` and `fstp` for the
 * player heading copy.  V2 lowers the same copy to an integer move, so the
 * module needs no FPU stub.  The module uses no assembly implementation.
 */
struct camera_view {
    int reserved_00;
    int reserved_04;
    int reserved_08;
    int saved_x;
    int reserved_10;
    int saved_z;
    int reserved_18;
    int reserved_1c;
    int reserved_20;
    int reserved_24;
    int reserved_28;
    int reserved_2c;
    int reserved_30;
    int yaw;
};

extern unsigned char DAT_00118e1d;
extern int DAT_0013ccc;
extern int DAT_0013cd0;
extern float DAT_0013cd4;
extern int DAT_0011fdcc;
extern int DAT_0011fdd4;
extern float DAT_0011fde8;

void camera_reference_update(struct camera_view *view)
{
    if ((DAT_00118e1d & 1) != 0 && (DAT_00118e1d & 0x20) == 0) {
        DAT_0013ccc = DAT_0011fdcc;
        DAT_0013cd4 = DAT_0011fde8;
        DAT_0013cd0 = DAT_0011fdd4;
    } else {
        DAT_0013ccc = view->saved_x;
        DAT_0013cd0 = view->saved_z;
        *(int *)&DAT_0013cd4 = view->yaw;
    }
}
