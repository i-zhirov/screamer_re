/* The rebuilt `object_gear_state` module.
 *
 * The original sits at linear 0x1db38 in STARTH.EXE and holds 16 instructions
 * and 47 bytes: the frame, the argument slot, the gear state read from
 * `+0x3f4`, the compare chain, the three stores to `+0x4e8`, and the epilogue.
 *
 * The function maps the gear state at `+0x3f4` to the gear field at `+0x4e8`:
 * the state 0 clears the field, the state 1 sets it to 0x8000, and the state 2
 * sets it to 0xff00.  The original uses a compare chain, so the rebuild uses
 * an if-else chain, which the compiler also holds without a jump table.
 */
void object_gear_state(void *object)
{
    unsigned int state;
    unsigned int *field;

    state = *(unsigned int *)((char *)object + 0x3f4);
    field = (unsigned int *)((char *)object + 0x4e8);
    if (state == 0) {
        *field = 0;
    } else if (state == 1) {
        *field = 0x8000;
    } else if (state == 2) {
        *field = 0xff00;
    }
}
