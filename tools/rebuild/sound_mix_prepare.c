/* The rebuilt `sound_mix_prepare` module.
 *
 * The original sits at linear 0x8aa60 in STARTH.EXE and holds 155 instructions
 * and 535 bytes up to the `ret` at 0x8ac76.  The next function starts at
 * 0x8ac78, so the store record keeps the 536-byte extent and sets no separate
 * function size.  The identity check uses 155 instructions.
 *
 * The only direct caller is `sound_effects_update` at 0x81ab0.  It calls this
 * function as the mix prepare, returns when the voice count at 0x7aa9c is zero
 * or negative, and then mixes the voices through 0x83500 with the table at
 * 0x45790.  The record takes the name `sound_mix_prepare` with the confidence
 * inferred.
 *
 * The entry calls the helper at 0x8a9e8.  That helper copies the camera basis
 * at +0x48 and the saved camera position at 0x7ad94 through 0x7ad9c into the
 * immediate fields of the code at 0x8aaca through 0x8ab18.  The original
 * patched its own code: the on-disk value 0xaaaabbbb is only the linker fill
 * value.  The rebuild reads the same matrix and the same position into local
 * variables at entry, which is equivalent because the loop does not change
 * them.
 *
 * The loop walks the source list at 0x40330.  The list holds 8-byte records of
 * the squared distance and the emitter record pointer.  For each record the
 * function:
 *
 *     - copies the distance and the emitter pointer to the 20-byte voice
 *       record at 0x42ef0 and transforms the emitter position at +8, +0xc, and
 *       +0x10 from world space into camera space by the Q14 basis
 *     - computes the radius from the square root of the squared distance,
 *       shifts it by two, and negates it
 *     - biases the radius by 0x320 for the flag 0x40000000, and by the object
 *       type at +0x32c: type 1 -0x5dc, type 2 -0x320, type 3 -0x1770, and type
 *       4 -0xc8
 *     - looks the object up through the handle table at 0x21540
 *     - skips a source behind the object radius at +0x00, a source below the
 *       far limit at 0xc7cf0, and a source whose projected box leaves the view
 *       rectangle
 *     - appends a 4-byte entry at 0x45790 with the voice byte offset and the
 *       radius
 *
 * The voice count at 0x7aa9c starts at zero and increments by 0x14 for each
 * entry.  The end of the loop replaces it with the entry count.
 *
 * The original has no EBP frame, so the module builds without `-of+`.  The
 * module takes the square root from the local `sound_mix_prepare_fpu.asm`
 * stub, because the harness links no math library.
 */
#define FLAG_40 0x40000000
#define FLAG_MASK 0xbfffffff
#define VOICE_STRIDE 0x14
#define BASIS_SHIFT 14
#define PROJECTION_SHIFT 17
#define NEAR_BIAS 0x320
#define TYPE_1_BIAS 0x5dc
#define TYPE_2_BIAS 0x320
#define TYPE_3_BIAS 0x1770
#define TYPE_4_BIAS 0xc8

extern double sqrt(double value);

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

struct mix_emitter {
    int object_index;
    int field_04;
    int position[3];
    int rest[11];
};

struct mix_source {
    int distance;
    struct mix_emitter *emitter;
};

struct mix_object {
    int size;
    int reserved[202];
    int type;
};

struct mix_voice {
    int distance;
    struct mix_emitter *emitter;
    int x;
    int y;
    int z;
};

struct mix_entry {
    unsigned short offset;
    short radius;
};

extern struct camera *camera_state;
extern int saved_position[3];
extern int sound_active_count;
extern int sound_voice_count;
extern int *DAT_00021540;
extern struct mix_source *sound_mix_sources;
extern struct mix_voice *sound_mix_voices;
extern struct mix_entry *sound_mix_entries;
extern int *sound_mix_projection;
extern int sound_mix_far_limit;
extern int sound_mix_center_x;
extern int sound_mix_center_y;
extern int sound_mix_bound_x;
extern int sound_mix_bound_y;

static int multiply_shift(int left, int right)
{
    unsigned int a;
    unsigned int b;
    unsigned int a0;
    unsigned int a1;
    unsigned int b0;
    unsigned int b1;
    unsigned int p0;
    unsigned int p1;
    unsigned int p2;
    unsigned int p3;
    unsigned int sum;
    unsigned int carry;
    unsigned int low;
    unsigned int high;
    unsigned int mid;

    a = (unsigned int)left;
    b = (unsigned int)right;
    a0 = a & 0xffff;
    a1 = a >> 16;
    b0 = b & 0xffff;
    b1 = b >> 16;
    p0 = a0 * b0;
    p1 = a0 * b1;
    p2 = a1 * b0;
    p3 = a1 * b1;
    sum = p1 + p2;
    carry = (sum < p1) ? 1u : 0u;
    mid = (sum & 0xffff) + (p0 >> 16);
    low = ((mid & 0xffff) << 16) | (p0 & 0xffff);
    high = p3 + (sum >> 16) + (carry << 16) + (mid >> 16);
    if (left < 0) {
        high -= b;
    }
    if (right < 0) {
        high -= a;
    }
    return (int)((low >> PROJECTION_SHIFT)
        | (high << (32 - PROJECTION_SHIFT)));
}

void sound_mix_prepare(void)
{
    struct camera *camera;
    struct mix_source *source;
    struct mix_emitter *emitter;
    struct mix_object *object;
    struct mix_voice *voice;
    int m0;
    int m1;
    int m2;
    int m3;
    int m4;
    int m5;
    int m6;
    int m7;
    int m8;
    int off0;
    int off1;
    int off2;
    int index;
    int kept;
    int a;
    int b;
    int c;
    int u;
    int v;
    int w;
    int radius;
    int size;
    int depth;
    int factor;
    int projected;

    camera = camera_state;
    m0 = camera->basis_b[0];
    m1 = camera->basis_b[3];
    m2 = camera->basis_b[6];
    m3 = camera->basis_b[1];
    m4 = camera->basis_b[4];
    m5 = camera->basis_b[7];
    m6 = camera->basis_b[2];
    m7 = camera->basis_b[5];
    m8 = camera->basis_b[8];
    off0 = saved_position[0];
    off1 = saved_position[1];
    off2 = saved_position[2];

    kept = 0;
    for (index = 0; index < sound_active_count; ++index) {
        source = &sound_mix_sources[index];
        emitter = (struct mix_emitter *)((unsigned int)source->emitter
            & FLAG_MASK);
        a = emitter->position[0] - off0;
        b = emitter->position[1] - off1;
        c = emitter->position[2] - off2;
        u = (a * m0 + b * m1 + c * m2) >> BASIS_SHIFT;
        v = (a * m3 + b * m4 + c * m5) >> BASIS_SHIFT;
        w = (a * m6 + b * m7 + c * m8) >> BASIS_SHIFT;

        voice = &sound_mix_voices[kept];
        voice->distance = source->distance;
        voice->emitter = source->emitter;
        voice->x = u;
        voice->y = v;
        voice->z = w;

        radius = -(((int)sqrt((double)source->distance)) >> 2);
        if ((unsigned int)source->emitter & FLAG_40) {
            radius += NEAR_BIAS;
        }

        object = (struct mix_object *)DAT_00021540[emitter->object_index];
        if (object->type == 4) {
            radius -= TYPE_4_BIAS;
        } else if (object->type == 1) {
            radius -= TYPE_1_BIAS;
        } else if (object->type == 2) {
            radius -= TYPE_2_BIAS;
        } else if (object->type == 3) {
            radius -= TYPE_3_BIAS;
        }

        size = object->size;
        depth = w - size;
        if (depth > 0) {
            continue;
        }
        factor = sound_mix_projection[depth];
        if (w + size <= sound_mix_far_limit) {
            continue;
        }
        projected = multiply_shift(u - size, factor) + sound_mix_center_x;
        if (projected < 0) {
            continue;
        }
        projected = multiply_shift(u + size, factor) + sound_mix_center_x;
        if (projected > sound_mix_bound_x) {
            continue;
        }
        projected = multiply_shift(v - size, factor) + sound_mix_center_y;
        if (projected < 0) {
            continue;
        }
        projected = multiply_shift(v + size, factor) + sound_mix_center_y;
        if (projected > sound_mix_bound_y) {
            continue;
        }

        sound_mix_entries[kept].offset = (unsigned short)(kept * VOICE_STRIDE);
        sound_mix_entries[kept].radius = (short)radius;
        kept++;
    }
    sound_voice_count = kept;
}
