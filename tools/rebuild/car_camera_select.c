/* The rebuilt `car_camera_select` module.
 *
 * The original sits at linear 0x1aec4 in STARTH.EXE and holds 52 instructions
 * and 159 bytes up to the `ret` at 0x1af62.  The next function starts at
 * 0x1af70, so the store record keeps the 172-byte extent and sets no separate
 * function size.  The identity check uses 52 instructions.
 *
 * The only direct call site at 0x19ee2 is the view mode dispatch at 0x19e78.
 * The dispatch table at 0x19e50 sends the render mode 8 to this function and
 * passes the view buffer in `eax`.  The player update at 0x19ab0 and
 * `player_view_setup` at 0x19ae8 both call the dispatch, so the module takes
 * the section `player_view_setup callees`.
 *
 * The function runs `car_camera` at 0x1a930 on every 0x590-byte car record at
 * `DAT_00110ab8` up to the count `DAT_0010f8ec`, and counts in
 * `DAT_0010f8ec` in the global `DAT_0010634c`.  It then commits the candidate
 * camera index `DAT_0010a1c` to the active index `DAT_00110a18` when the
 * active car sets the flag `+0x564` and the two indices differ.  The flag is
 * the changed-target marker of `car_camera`.
 *
 * The function finally copies the 0x98-byte view block at `+0x4f8` of the
 * active car record into the view buffer.  The original inlines the copy as a
 * `movsd` and `movsb` sequence, so the module uses `memcpy` with the same
 * constant size.
 *
 * `player_view_setup` cycles the candidate index over the car records when the
 * chosen view slot 4 runs the camera scan, so the module names this function
 * `car_camera_select` with the confidence inferred.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses no floating-point instruction, so the module needs no FPU
 * stub, and the module uses no assembly.
 */
extern unsigned char DAT_00110ab8[];
extern int DAT_0010f8ec;
extern int DAT_0010634c;
extern int DAT_0010a18;
extern int DAT_0010a1c;
extern void car_camera(char *car);
extern void *memcpy(void *target, const void *source, unsigned int count);

void car_camera_select(char *view)
{
    char *car;
    char *source;

    DAT_0010634c = 0;
    while (DAT_0010634c < DAT_0010f8ec) {
        car = (char *)DAT_00110ab8 + DAT_0010634c * 0x590;
        car_camera(car);
        DAT_0010634c++;
    }

    car = (char *)DAT_00110ab8 + DAT_0010a18 * 0x590;
    if (*(int *)(car + 0x564) != 0 && DAT_0010a18 != DAT_0010a1c) {
        DAT_0010a18 = DAT_0010a1c;
    }

    source = (char *)DAT_00110ab8 + DAT_0010a18 * 0x590 + 0x4f8;
    memcpy(view, source, 0x98);
}
