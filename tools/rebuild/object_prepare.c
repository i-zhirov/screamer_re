/* The rebuilt `object_prepare` module.
 *
 * The original sits at linear 0x1c79c in STARTH.EXE and holds 16 instructions
 * and 49 bytes: the frame, the argument slot, the preparation call at
 * `0x1db94`, the guard on the state at `+0x414`, the second preparation call
 * at `0x1ce38`, the third call at `0x1cbac`, and the epilogue.
 *
 * The function runs the preparation steps: the first step always runs, the
 * second runs when the state at `+0x414` is set, and the third step always
 * runs.
 */
extern void object_prepare_0(void *object);
extern void object_prepare_1(void *object);
extern void object_prepare_2(void *object);

void object_prepare(void *object)
{
    object_prepare_0(object);
    if (*(unsigned int *)((char *)object + 0x414) != 0) {
        object_prepare_1(object);
    }
    object_prepare_2(object);
}
