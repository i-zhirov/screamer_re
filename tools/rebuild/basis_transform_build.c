/* The rebuilt `basis_transform_build` module.
 *
 * The original sits at linear 0x8b610 in STARTH.EXE and holds 65 instructions
 * and 222 bytes up to the `ret` at 0x8b6ed.  The next recorded function starts
 * at 0x8bc7c, so the store record keeps the 1644-byte extent and sets no
 * separate function size.  The identity check uses 65 instructions.
 *
 * The function builds the two basis matrices of one object record.  The record
 * holds three angle indices at `+0x00`, `+0x04`, and `+0x08`, the position at
 * `+0x0c`, `+0x10`, and `+0x14`, the forward basis at `+0x18`, the saved
 * position at `+0x3c`, `+0x40`, and `+0x44`, and the inverse basis at `+0x48`.
 *
 * An angle index selects an 8-byte table entry at 0x203d0.  The entry holds a
 * signed 16-bit cosine in the low word at `+0x00` and a signed 16-bit sine in
 * the low word at `+0x04`.  The function sign-extends each value and shifts it
 * one bit to the right with the arithmetic shift, which converts the Q15 value
 * to Q14.  The first call at 0x81fd0 builds the forward basis at `+0x18` from
 * the three angles.
 *
 * The second pass forms the complement index `0x7ff - angle` for each angle
 * and calls 0x82074, which builds the inverse basis at `+0x48`.  The function
 * then copies the position to the saved position at `+0x3c`, `+0x40`, and
 * `+0x44`.  It negates the position and calls 0x81f80, which multiplies the
 * inverse basis by the negated position and stores the inverse translation at
 * `+0x24`, `+0x28`, and `+0x2c` of the inverse basis.
 *
 * The direct caller `camera_update` at 0x8ac78 calls the function at 0x8ac94
 * with the camera record.  The caller `sound_voice_setup` at 0x8adac calls the
 * function at 0x8aea0 with the voice context.  The caller
 * `sound_source_frame_prepare` at 0x8affc calls the function at 0x8b073 with
 * the source pose.  The three callers support the generic name
 * `basis_transform_build`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has no EBP frame.  It uses `ebp` as a scratch register and does
 * not save it, so the module builds without `-of+`.  The original uses no
 * floating-point instruction, so the module needs no FPU stub.
 */
#define MIRROR_BASE 0x7ff
#define FIXED_SHIFT 1

struct angle_entry {
    short cosine;
    short reserved_00;
    short sine;
    short reserved_04;
};

struct basis_object {
    int angle0;
    int angle1;
    int angle2;
    int position_x;
    int position_y;
    int position_z;
    int forward[9];
    int saved_x;
    int saved_y;
    int saved_z;
    int inverse[12];
};

extern struct angle_entry angle_table[];

extern void basis_rotation_build(int *matrix, int cos0, int sin0,
    int cos1, int sin1, int cos2, int sin2);
extern void basis_rotation_mirror(int *matrix, int cos0, int sin0,
    int cos1, int sin1, int cos2, int sin2);
extern void basis_vector_transform(int *matrix, int x, int y, int z);

void basis_transform_build(struct basis_object *object)
{
    basis_rotation_build(object->forward,
        angle_table[object->angle0].cosine >> FIXED_SHIFT,
        angle_table[object->angle0].sine >> FIXED_SHIFT,
        angle_table[object->angle1].cosine >> FIXED_SHIFT,
        angle_table[object->angle1].sine >> FIXED_SHIFT,
        angle_table[object->angle2].cosine >> FIXED_SHIFT,
        angle_table[object->angle2].sine >> FIXED_SHIFT);

    basis_rotation_mirror(object->inverse,
        angle_table[MIRROR_BASE - object->angle0].cosine >> FIXED_SHIFT,
        angle_table[MIRROR_BASE - object->angle0].sine >> FIXED_SHIFT,
        angle_table[MIRROR_BASE - object->angle1].cosine >> FIXED_SHIFT,
        angle_table[MIRROR_BASE - object->angle1].sine >> FIXED_SHIFT,
        angle_table[MIRROR_BASE - object->angle2].cosine >> FIXED_SHIFT,
        angle_table[MIRROR_BASE - object->angle2].sine >> FIXED_SHIFT);

    object->saved_x = object->position_x;
    object->saved_y = object->position_y;
    object->saved_z = object->position_z;
    basis_vector_transform(object->inverse,
        -object->position_x, -object->position_y, -object->position_z);
}
