/* The rebuilt `sound_source_frame_select` module.
 *
 * The original sits at linear 0x8b1d4 in STARTH.EXE and holds 103 instructions
 * and 293 bytes up to the `ret` at 0x8b2f8.  The next function starts at
 * 0x8b2fc, so the store record keeps the 1084-byte extent to the next recorded
 * function at 0x8b610 and sets no separate function size.  The identity check
 * uses 103 instructions.
 *
 * The only direct caller is at 0x8b0e0 in the per-source frame prepare at
 * 0x8affc.  That helper runs from the frame prepare at 0x8af08, which
 * `sound_effects_update` at 0x81ab0 calls in the per-frame loop.  The record
 * takes the section `object_update_all callees` with the sound chain.
 *
 * The context record holds the voice record at `+0x0c`, the current object at
 * `+0x14`, the frame index at `+0x08`, the pointer to the frame table entry at
 * `+0xb4`, and the table base at `+0xb8`.
 *
 * The object holds three signed shorts at `+0x04`, `+0x06`, and `+0x08` and a
 * name or identifier at `+0x68`.  The voice record holds the camera matrix at
 * `+0x118`, the frame pointer at `+0x00`, and the distance at `+0x10`.
 *
 * The function transforms the three shorts by the Q14 matrix, forms the
 * square root of the squared camera distance, and truncates it toward the
 * nearest integer as the original `fistp` does.  It negates the result.  The
 * name byte `A` or `a` skips the bias, and a zero second byte with `i` or `x`
 * in the third byte biases the distance by -4000 or -1000.
 *
 * The raw negative distance selects one of three frame table bands at 0, 0x104
 * or 0x208 from the two limits at `distance_limit_near` and
 * `distance_limit_far`.  The selected entry is an offset from the table base.
 * A zero offset gives the null frame.  The function stores the frame in the
 * voice record and back in the object field, and stores the biased distance in
 * the per-index distance table.
 *
 * The original uses raw `fistp`, which rounds to nearest.  This module keeps
 * the shared `sound_source_frame_select_fpu.asm` `__CHP` no-op, so the C
 * conversion keeps the same rounding, and the local stub supplies `sqrt`.
 */
#define BAND_NEAR 0
#define BAND_MIDDLE 0x104
#define BAND_FAR 0x208
#define NAME_BIAS_I 4000
#define NAME_BIAS_X 1000
#define MATRIX_SHIFT 14

struct source_distance {
    short id;
    short distance;
};

extern int distance_limit_near;
extern int distance_limit_far;
extern struct source_distance source_distances[];

extern double sqrt(double value);

void sound_source_frame_select(char *context)
{
    char *voice;
    char *object;
    short *point;
    int *matrix;
    int vx;
    int vy;
    int vz;
    int px;
    int py;
    int pz;
    double squared;
    int raw;
    int adjusted;
    unsigned int id;
    int band;
    int *entry;
    int offset;
    char *frame;
    int index;

    voice = *(char **)(context + 0x0c);
    object = *(char **)(context + 0x14);
    point = (short *)(object + 4);
    matrix = (int *)(voice + 0x118);

    vx = point[0];
    vy = point[1];
    vz = point[2];

    px = ((vx * matrix[0] + vy * matrix[3] + vz * matrix[6]) >> MATRIX_SHIFT)
        + matrix[9];
    py = ((vx * matrix[1] + vy * matrix[4] + vz * matrix[7]) >> MATRIX_SHIFT)
        + matrix[10];
    pz = ((vx * matrix[2] + vy * matrix[5] + vz * matrix[8]) >> MATRIX_SHIFT)
        + matrix[11];

    squared = (double)px * px + (double)py * py + (double)pz * pz;
    raw = -(int)sqrt(squared);
    adjusted = raw;

    id = *(unsigned int *)(object + 0x68);
    if ((id & 0xff) != 0x41 && (id & 0xff) != 0x61
        && ((id >> 8) & 0xff) == 0) {
        if (((id >> 16) & 0xff) == 0x69) {
            adjusted = raw - NAME_BIAS_I;
        } else if (((id >> 16) & 0xff) == 0x78) {
            adjusted = raw - NAME_BIAS_X;
        }
    }

    *(int *)(voice + 0x10) = adjusted;

    if (raw > distance_limit_near) {
        band = BAND_NEAR;
    } else if (raw > distance_limit_far) {
        band = BAND_MIDDLE;
    } else {
        band = BAND_FAR;
    }

    entry = (int *)(*(char **)(context + 0xb4) + band);
    offset = *entry;
    frame = 0;
    if (offset != 0) {
        frame = (char *)(offset + *(int *)(context + 0xb8));
    }
    *(char **)voice = frame;
    *(char **)(context + 0x14) = frame;

    index = *(int *)(context + 8);
    source_distances[index].distance = (short)adjusted;
}
