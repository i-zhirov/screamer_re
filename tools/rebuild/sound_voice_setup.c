/* The rebuilt `sound_voice_setup` module.
 *
 * The original sits at linear 0x8adac in STARTH.EXE and holds 71 instructions
 * and 279 bytes up to the first `ret` at 0x8aec2.  The function has a second
 * path that ends at the `ret` at 0x8af05.  The next function starts at
 * 0x8af08, so the store record keeps the 348-byte extent and sets no separate
 * function size.  The identity check uses 71 instructions.
 *
 * The only direct caller is the frame prepare at 0x8af08.  That helper runs on
 * the per-frame path of `sound_effects_update` at 0x81ab0.  The record takes
 * the section `object_update_all callees`.
 *
 * The function reads one 4-byte entry from the voice entry list at
 * `sound_voice_entry`.  The entry holds the byte offset of a 20-byte voice
 * record at `sound_voice_records`.  The record holds the emitter pointer at
 * `+0x04`.  The function clears bit 0x40000000 of the emitter pointer, and for
 * a set bit it stores the biased radius at `+0xc8` of the voice context.
 *
 * The emitter holds the object index at `+0x00`, the flag word at `+0x04`,
 * the position at `+0x08`, `+0x0c`, and `+0x10`, the three basis values at
 * `+0x14`, `+0x18`, and `+0x1c`, the level at `+0x20`, and one value at
 * `+0x3c`.  The function copies the level to the globals 0x74524 and 0x74534,
 * copies the three basis values and the camera-relative position to the voice
 * context, and builds the camera basis at `+0x30`.
 *
 * Bit 0x8000 of the emitter flag word selects the second basis builder.  That
 * builder also receives the emitter and sets the flag 0x7b518.  The first
 * builder takes only the basis.  After the call the function reads the object
 * through `sound_handle_table`, stores the object at `+0x00`, the object count
 * at `+0x04`, and zero at `+0x08`.
 *
 * A non-negative object field at `+0x10` becomes `field * scale >> 16 * 0x30`
 * at `+0xd4`.  A negative field stays unchanged.
 *
 * The original has no EBP frame, so the module builds without `-of+`.  The
 * original uses no floating-point instruction, so the module needs no FPU
 * stub.
 */
#define NEAR_FLAG 0x40000000
#define NEAR_MASK 0xbfffffff
#define EVENT_FLAG 0x8000
#define FIXED_SCALE 0x30

struct voice_emitter {
    int object_index;
    int flags;
    int x;
    int y;
    int z;
    int basis0;
    int basis1;
    int basis2;
    int level;
    int reserved_24[6];
    int field_3c;
};

struct voice_object {
    int reserved_00[4];
    int field_10;
    int field_14;
};

extern unsigned char sound_voice_context[];
extern unsigned char sound_voice_records[];
extern unsigned char *sound_voice_entry;
extern void **sound_handle_table;
extern int sound_emitter_scale;
extern int sound_camera_x;
extern int sound_camera_y;
extern int sound_camera_z;
extern int sound_voice_volume;
extern int sound_voice_shift;
extern int sound_voice_field;
extern int sound_voice_flag;

extern void sound_basis_build(void *basis);
extern void sound_basis_build_event(void *basis, void *emitter);

void sound_voice_setup(void)
{
    struct voice_emitter *emitter;
    struct voice_object *object;
    unsigned short offset;
    int value;
    int raw;

    *(int *)(sound_voice_context + 0xc8) = -1;
    offset = *(unsigned short *)sound_voice_entry;
    sound_voice_entry += 4;
    raw = *(int *)(sound_voice_records + offset + 4);
    *(int *)(sound_voice_context + 0xd8) = raw;
    emitter = (struct voice_emitter *)(raw & NEAR_MASK);
    if (raw & NEAR_FLAG) {
        *(int *)(sound_voice_context + 0xc8) = (int)emitter + 0x40;
    }

    sound_voice_field = emitter->field_3c;
    value = emitter->level;
    sound_voice_volume = value;
    sound_voice_shift = value >> 2;
    if (value < 0) {
        value = sound_emitter_scale;
        sound_voice_volume = value;
        sound_voice_shift = value & 0x3fff;
    }

    *(int *)(sound_voice_context + 0x30) = emitter->basis0;
    *(int *)(sound_voice_context + 0x34) = emitter->basis1;
    *(int *)(sound_voice_context + 0x38) = emitter->basis2;
    *(int *)(sound_voice_context + 0x3c) = emitter->x - sound_camera_x;
    *(int *)(sound_voice_context + 0x40) = emitter->y - sound_camera_y;
    *(int *)(sound_voice_context + 0x44) = emitter->z - sound_camera_z;

    sound_voice_flag = 0;
    value = emitter->flags & EVENT_FLAG;
    object = (struct voice_object *)sound_handle_table[emitter->object_index];
    *(int *)(sound_voice_context + 0x00) = (int)object;
    *(int *)(sound_voice_context + 0x04) = object->field_14;
    *(int *)(sound_voice_context + 0x08) = 0;
    if (value != 0) {
        sound_basis_build_event(sound_voice_context + 0x30, emitter);
    } else {
        sound_basis_build(sound_voice_context + 0x30);
    }

    object = (struct voice_object *)*(int *)(sound_voice_context + 0x00);
    value = object->field_10;
    if (value >= 0) {
        value = (value * sound_emitter_scale) >> 16;
        value = value * FIXED_SCALE;
    }
    *(int *)(sound_voice_context + 0xd4) = value;
}
