/* The rebuilt `object_player_input` module.
 *
 * The original sits at linear 0x1c760 in STARTH.EXE and holds 18 instructions
 * and 58 bytes: the frame, the argument slot, the proximity check, the guard on
 * the state at `+0x2f8` and the input bits `DAT_00118e1c`, the drive step, the
 * revolution step, and the epilogue.
 *
 * The function runs the proximity check, and when the state at `+0x2f8` is
 * clear and the input bits hold bit 0 or bit 2, it runs the drive step.  The
 * revolution step always runs.
 */
extern void object_proximity_check(void *object);
extern void object_drive_step(void *object);
extern void object_revolution_step(void *object);
extern unsigned char DAT_00118e1c;

void object_player_input(void *object)
{
    object_proximity_check(object);
    if (*(unsigned int *)((char *)object + 0x2f8) == 0 && (DAT_00118e1c & 5)) {
        object_drive_step(object);
    }
    object_revolution_step(object);
}
