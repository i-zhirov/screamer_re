/* The rebuilt `sound_source_frame_prepare` module.
 *
 * The original sits at linear 0x8affc in STARTH.EXE and holds 74 instructions
 * and 234 bytes up to the `ret` at 0x8b0e5.  The next function starts at
 * 0x8b0e8.  The next recorded function starts at 0x8b1d4, so the store record
 * keeps the 472-byte extent and sets no separate function size.  The identity
 * check uses 74 instructions.
 *
 * The only direct caller at 0x8af35 is the per-source frame prepare at
 * 0x8af08.  That helper runs on the per-frame path of `sound_effects_update`
 * at 0x81ab0, so the record takes the section `object_update_all callees`.
 *
 * The context record holds the frame table base at `+0x00`, the frame index at
 * `+0x08`, the voice record at `+0x0c`, the current object at `+0x14`, the
 * frame table entry pointer at `+0xb4`, the table base at `+0xb8`, and the
 * source record at `+0xc8`.
 *
 * The function resets the source distance identifier, points the frame table
 * entry at `table + index * 4 + 0x18`, and takes the object offset from that
 * entry.  It stores the current object in the context and in the voice record.
 *
 * The function copies six dwords from the source record at `+0x08` through
 * `+0x1c` to the voice record at `+0x10` through `+0x24`.  It runs the flag
 * helper at 0x8afb2.  A non-negative sample offset at 0x74528 runs the view
 * helper at 0x8afa0 and the pose copy at 0x8afc4; a negative offset runs the
 * index transform at 0x8b610 on the six copied dwords.
 *
 * The function composes four Q14 matrices through `matrix_compose` at
 * 0x82258:
 *
 *     1  the context block `+0x48` with the voice block `+0x28` to `+0xa0`
 *     2 the camera basis `+0x48` with the result `+0xa0` to `+0x118`
 *     3 the context block `+0x78` with the camera basis `+0x18` to `+0xd0`
 *     4 the voice block `+0x58` with the result `+0xd0` to `+0x148`
 *
 * The function then calls `sound_source_frame_select` at 0x8b1d4 with the
 * context.
 *
 * The caller, the per-source loop, the source record copy, and the frame
 * select call support the name `sound_source_frame_prepare`.  The original has
 * no symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has no EBP frame setup and saves ebp because it uses the
 * register as the voice pointer, so the module needs no `-of+`.  The original
 * uses no floating-point instruction and calls no FPU helper.
 */
#define SOURCE_DISTANCE_ID 0
#define FRAME_TABLE_BIAS 0x18

struct source_distance {
    short id;
    short distance;
};

extern struct source_distance source_distances[];
extern int sound_sample_offset;
extern char *camera_state;

extern void sub_8afb2(void);
extern void sub_8afa0(void);
extern void sub_8afc4(char *voice, char *table);
extern void sub_8b610(int *values);
extern void matrix_compose(int *source, int *matrix, int *destination);
extern void sound_source_frame_select(char *context);

void sound_source_frame_prepare(char *context)
{
    char *table;
    char *voice;
    char *source;
    char *object;
    char *entry;
    int index;
    int value;
    int count;

    voice = *(char **)(context + 0x0c);
    table = *(char **)(context + 0x00);
    index = *(int *)(context + 0x08);
    source_distances[index].id = (short)index;

    entry = table + index * 4 + FRAME_TABLE_BIAS;
    *(char **)(context + 0xb4) = entry;
    *(char **)(context + 0xb8) = table;
    object = table + *(int *)entry;
    *(char **)(context + 0x14) = object;
    *(char **)voice = object;

    source = *(char **)(context + 0xc8);
    for (count = 0; count < 6; count++) {
        *(int *)(voice + 0x10 + count * 4) =
            *(int *)(source + 0x08 + count * 4);
    }

    sub_8afb2();
    value = sound_sample_offset;
    if (value >= 0) {
        sub_8afa0();
        sub_8afc4(voice, table);
    } else {
        sub_8b610((int *)(voice + 0x10));
    }

    matrix_compose((int *)(context + 0x48), (int *)(voice + 0x28),
        (int *)(voice + 0xa0));
    matrix_compose((int *)(camera_state + 0x48), (int *)(voice + 0xa0),
        (int *)(voice + 0x118));
    matrix_compose((int *)(context + 0x78), (int *)(camera_state + 0x18),
        (int *)(voice + 0xd0));
    matrix_compose((int *)(voice + 0x58), (int *)(voice + 0xd0),
        (int *)(voice + 0x148));
    sound_source_frame_select(context);
}
