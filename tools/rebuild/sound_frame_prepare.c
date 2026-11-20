/* The rebuilt `sound_frame_prepare` module.
 *
 * The original sits at linear 0x8af08 in STARTH.EXE and holds 22 instructions
 * and 98 bytes up to the first `ret` at 0x8af69.  The function has a second
 * path that ends at the `ret` at 0x8af9f.  The next recorded function starts at
 * 0x8afa0, so the store record keeps the 152-byte extent and sets no separate
 * function size.  The identity check uses 22 instructions.
 *
 * The only direct caller is the per-frame loop of `sound_effects_update` at
 * 0x81ab0, so the record takes the section `object_update_all callees`.  The
 * caller uses the name `sound_frame_prepare` for the call at 0x8af08.
 *
 * The voice context at 0x74454 holds the frame table base at `+0x00`, the voice
 * count at `+0x04`, the frame index at `+0x08`, the voice record at `+0x0c`,
 * and the source record at `+0xc8`.  `sound_voice_setup` at 0x8adac writes the
 * voice count.
 *
 * The function first calls `sound_voice_setup`.  It clears the frame index and
 * takes the voice count as the loop count.  It then points the voice record at
 * 0x7453c.
 *
 * A set bit 0x40000000 of the source record selects the second path.  That
 * path calls the near helper at 0x8b0e8 and does not advance the source record.
 * The first path calls `sound_source_frame_prepare` at 0x8affc and advances the
 * source record by 0x20.  Both paths advance the voice record by 0x1f4 and
 * increment the frame index for each voice.
 *
 * The function then calls the voice mix at 0x83500 with the table 0x7a6e4 and
 * the voice count.
 *
 * The original has no EBP frame, so the module builds without `-of+`.  The
 * original uses no floating-point instruction, so the module needs no FPU stub.
 * The module uses no assembly implementation.
 */
#define NEAR_FLAG 0x40000000
#define VOICE_RECORD_BASE 0x7453c
#define VOICE_RECORD_STEP 0x1f4
#define SOURCE_RECORD_STEP 0x20
#define MIX_TABLE 0x7a6e4

struct sound_frame_context {
    int frame_base;             /* +0x00 */
    int voice_count;            /* +0x04 */
    int frame_index;            /* +0x08 */
    int voice_record;           /* +0x0c */
    int reserved_10[46];        /* +0x10 through +0xc4 */
    int source_record;          /* +0xc8 */
};

extern struct sound_frame_context sound_voice_context;

extern void sound_voice_setup(void);
extern void sound_source_frame_prepare(struct sound_frame_context *context);
extern void sub_8b0e8(struct sound_frame_context *context);
extern void sound_voice_mix(void *table, int count);

void sound_frame_prepare(void)
{
    struct sound_frame_context *context = &sound_voice_context;
    int count;

    sound_voice_setup();
    context->frame_index = 0;
    count = context->voice_count;
    context->voice_record = VOICE_RECORD_BASE;
    if ((context->source_record & NEAR_FLAG) != 0) {
        do {
            sub_8b0e8(context);
            context->voice_record += VOICE_RECORD_STEP;
            context->frame_index++;
        } while (--count != 0);
    } else {
        do {
            sound_source_frame_prepare(context);
            context->source_record += SOURCE_RECORD_STEP;
            context->voice_record += VOICE_RECORD_STEP;
            context->frame_index++;
        } while (--count != 0);
    }
    sound_voice_mix((void *)MIX_TABLE, context->voice_count);
}
