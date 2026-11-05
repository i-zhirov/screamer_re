/* The rebuilt `camera_update` module.
 *
 * The original sits at linear 0x8ac78 in STARTH.EXE and holds 64 instructions
 * and 225 bytes: the all-register save, the four segment saves, the store of
 * the argument in the global `0x7a7ac`, the two calls at 0x8b610 and 0x82c14,
 * the copy of the position vector, the clear of the velocity and the position,
 * the nine scaled basis values, and the epilogue.
 *
 * The argument is a pointer to a camera record.  The record holds a 3x3
 * integer basis at `+0x18`, the position vector at `+0x3c`, a second 3x3
 * integer basis at `+0x48`, and the velocity vector at `+0x6c`.
 *
 * The function stores the pointer in the global `camera_state` and calls the
 * transform at 0x8b610 with the pointer and the projection at 0x82c14, which
 * reads the same global.  It then copies the position vector to the global
 * `saved_position`, clears the velocity vector, clears the position vector, and
 * stores the nine values of the second basis scaled by `camera_scale` into the
 * global `scaled_basis`.
 *
 * The original visits the nine basis values in the order 0, 3, 6, 1, 4, 7, 2,
 * 5, 8, so the rebuild writes the same order.
 *
 * The original has no EBP frame, so the module builds without `-of+`.
 *
 * The float constant arrives through the global `camera_scale`.  The harness
 * sets the exact bit pattern.  This keeps the float math out of the constant
 * segment, which the shared harness image does not load for this module.
 */
struct camera {
    int field_00;
    int field_04;
    int field_08;
    int field_0c;
    int field_10;
    int field_14;
    int basis_a[9];
    int position[3];
    int basis_b[9];
    int velocity[3];
};

extern struct camera *camera_state;
extern int saved_position[3];
extern float camera_scale;
extern float scaled_basis[9];

extern void camera_transform(struct camera *camera);
extern void camera_project(void);

void camera_update(struct camera *camera)
{
    camera_state = camera;
    camera_transform(camera);
    camera_project();
    saved_position[0] = camera->position[0];
    saved_position[1] = camera->position[1];
    saved_position[2] = camera->position[2];
    camera->velocity[0] = 0;
    camera->velocity[1] = 0;
    camera->velocity[2] = 0;
    camera->position[0] = 0;
    camera->position[1] = 0;
    camera->position[2] = 0;
    scaled_basis[0] = (float)camera->basis_b[0] * camera_scale;
    scaled_basis[3] = (float)camera->basis_b[3] * camera_scale;
    scaled_basis[6] = (float)camera->basis_b[6] * camera_scale;
    scaled_basis[1] = (float)camera->basis_b[1] * camera_scale;
    scaled_basis[4] = (float)camera->basis_b[4] * camera_scale;
    scaled_basis[7] = (float)camera->basis_b[7] * camera_scale;
    scaled_basis[2] = (float)camera->basis_b[2] * camera_scale;
    scaled_basis[5] = (float)camera->basis_b[5] * camera_scale;
    scaled_basis[8] = (float)camera->basis_b[8] * camera_scale;
}
