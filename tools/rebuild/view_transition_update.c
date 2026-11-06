/* The rebuilt `view_transition_update` module.
 *
 * The original sits at linear 0x19f00 in STARTH.EXE and holds 178 instructions
 * and 577 bytes up to the `ret` at 0x1a140.  The next function starts at
 * 0x1a144, so the store record keeps the 580-byte extent and sets no function
 * size.  The identity check uses 178 instructions.
 *
 * The two direct callers are the player update at 0x19acc and the car camera
 * at 0x1aeb3.  The player update runs this function as its third view step on
 * the player view object `DAT_00110a20`.  The car camera runs it on the view
 * block at `+0x4f8` of a 0x590-byte car record.  The view setup at 0x19ae8
 * starts the transition, so the record takes the section
 * `player_update callees`.
 *
 * The setup sets the flag `+0x54` and the flag `+0x58` when it selects a new
 * view.  A set flag `+0x54` makes this function form the position deltas
 * `+0x24`, `+0x28`, and `+0x2c` from the copy at `+0x18` minus the base at
 * `+0x00`, and the angle deltas `+0x3c`, `+0x40`, and `+0x44` from the saved
 * angles at `+0x48` minus the base angles at `+0x30`.  It then clears the flag.
 *
 * A set flag `+0x58` makes this function move the three angle deltas toward
 * zero through the approach helper at 0x22018 with the step table index 0x19.
 * It scales the three position deltas by the frame factor at
 * `DAT_0010cf08 + 0x168`.  When the angle cosines stay above 0.99995 and the
 * three scaled deltas stay below `DAT_00118e04 * 200.0` in absolute value, the
 * view has settled: the function sets the screen mode 320 by the view distance
 * with the horizon at 65 percent of the distance, copies the view video mode
 * to `DAT_001100c0`, and clears the flag `+0x58`.
 *
 * The original calls `cos` through 0x4da66 and the truncating helper at 0x4d9f4.
 * This harness links no math library, so the local assembly stub supplies
 * `cos` from the hardware and makes `__CHP` truncate toward zero.  The three
 * constants arrive through globals, because the minimal image does not load
 * the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
struct player_view {
    int x;
    int y;
    int z;
    int field_0c;
    int field_10;
    int field_14;
    int copy_x;
    int copy_y;
    int copy_z;
    int delta_x;
    int delta_y;
    int delta_z;
    float angle_x;
    float angle_y;
    float angle_z;
    float delta_ax;
    float delta_ay;
    float delta_az;
    float saved_x;
    float saved_y;
    float saved_z;
    int flag_54;
    int flag_58;
    int field_5c;
    int field_60;
    int field_64;
    int field_68;
    int field_6c;
    int field_70;
    int field_74;
    int field_78;
    int flag_7c;
    int flag_80;
    int index;
    int mode;
    int video;
    int distance;
};

extern double cos(double value);

extern void object_angle_approach(float *target, float value, int index);
extern void screen_mode_set(unsigned int width, unsigned int height,
                            unsigned int value);

extern float *DAT_0010cf08;
extern int DAT_001100c0;
extern float DAT_00118e04;

extern double view_transition_cos_limit;
extern float view_transition_distance_limit;
extern double view_transition_height_scale;

void view_transition_update(struct player_view *view)
{
    int height;

    if (view->flag_54 != 0) {
        view->flag_54 = 0;
        view->delta_x = view->copy_x - view->x;
        view->delta_y = view->copy_y - view->y;
        view->delta_z = view->copy_z - view->z;
        view->delta_ax = view->saved_x - view->angle_x;
        view->delta_ay = view->saved_y - view->angle_y;
        view->delta_az = view->saved_z - view->angle_z;
    }
    if (view->flag_58 != 0) {
        object_angle_approach(&view->delta_ax, 0.0f, 0x19);
        object_angle_approach(&view->delta_ay, 0.0f, 0x19);
        object_angle_approach(&view->delta_az, 0.0f, 0x19);
        view->delta_x = (int)((float)view->delta_x
            * *(float *)((char *)DAT_0010cf08 + 0x168));
        view->delta_y = (int)((float)view->delta_y
            * *(float *)((char *)DAT_0010cf08 + 0x168));
        view->delta_z = (int)((float)view->delta_z
            * *(float *)((char *)DAT_0010cf08 + 0x168));
        if (cos((double)view->delta_ax) > view_transition_cos_limit
            && cos((double)view->delta_ay) > view_transition_cos_limit
            && cos((double)view->delta_az) > view_transition_cos_limit
            && (view->delta_x < 0 ? -view->delta_x : view->delta_x)
                < DAT_00118e04 * view_transition_distance_limit
            && (view->delta_y < 0 ? -view->delta_y : view->delta_y)
                < DAT_00118e04 * view_transition_distance_limit
            && (view->delta_z < 0 ? -view->delta_z : view->delta_z)
                < DAT_00118e04 * view_transition_distance_limit) {
            height = (int)((double)view->distance
                * view_transition_height_scale);
            screen_mode_set(0x140, view->distance, height);
            DAT_001100c0 = view->video;
            view->flag_58 = 0;
        }
    }
}
