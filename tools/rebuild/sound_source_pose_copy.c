/* The rebuilt `sound_source_pose_copy` module.
 *
 * The original sits at linear 0x8afc4 in STARTH.EXE and holds 25 instructions
 * and 56 bytes up to the `ret` at 0x8affb.  The next function starts at
 * 0x8affc, so the 56-byte extent equals the body.  The store record keeps the
 * extent and sets no separate function size.  The identity check uses 25
 * instructions.
 *
 * The only direct caller at 0x8b067 is the per-source frame prepare at 0x8affc.
 * That helper runs from the frame prepare at 0x8af08, which
 * `sound_effects_update` at 0x81ab0 calls in the per-frame loop.  The record
 * takes the section `object_update_all callees`.
 *
 * The caller holds the voice record in ebp and the frame table base in ebx.
 * The original reads both registers directly and saves them with `pushal`, so
 * the two inputs are register inputs, not stack arguments.  This module states
 * them as two explicit parameters, because the C build uses its own frame.
 *
 * The source pointer forms from the global state pointer at 0x74468.  The
 * dword at state `+0x0c` is an offset from the frame table, and the sample
 * offset at 0x74528 is added after the constant 0x1c.  The function copies
 * twelve signed 16-bit words to the voice record at `+0x28` and the next
 * twelve signed 16-bit words to `+0x58`, and sign-extends every word to a
 * 32-bit int.  The two blocks feed the two Q14 pose matrices of the caller.
 *
 * The original has no floating-point instruction and makes no math call.
 */
#define POSE_ELEMENTS 12
#define POSE_FIRST 0x28
#define POSE_SECOND 0x58
#define SOURCE_BIAS 0x1c

extern void *sound_state;
extern int sound_sample_offset;

void sound_source_pose_copy(char *voice, char *table)
{
    short *source;
    int index;

    source = (short *)(table
        + *(int *)((char *)sound_state + 0x0c)
        + SOURCE_BIAS
        + sound_sample_offset);
    for (index = 0; index < POSE_ELEMENTS; index++) {
        *(int *)(voice + POSE_FIRST + index * 4) = (int)source[index];
    }
    for (index = 0; index < POSE_ELEMENTS; index++) {
        *(int *)(voice + POSE_SECOND + index * 4) =
            (int)source[POSE_ELEMENTS + index];
    }
}
