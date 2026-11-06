/* The rebuilt `sound_effects_update` module.
 *
 * The original starts at linear 0x81ab0 in STARTH.EXE and holds 51
 * instructions and 235 bytes up to the `ret` at 0x81b9a.  The entry saves all
 * registers and the four segment registers, calls the two setup helpers at
 * `0x83ca8` and `0x83354`, and joins the shared service body at `0x81ac6`.
 *
 * The neighbour `sound_update` starts at `0x81a70` and joins the same body at
 * `0x81ac6`.  This module starts at `0x81ab0`, so it holds the two setup calls
 * of its own entry and the shared body, and it does not hold the `0x81a70`
 * entry.
 *
 * The function stores the driver pointer in the global `0x7451c` and calls the
 * timer helper, the sequence helper that sets the active count `0x72488`, and
 * the source update.  A zero or negative active count returns at once.  The
 * function then calls the mix prepare and returns when the voice count
 * `0x7aa9c` is zero or negative.  Otherwise it mixes the voices, sets the song
 * pointer `0x7251c` and the frame repeat `0x72c10`, and runs the frame loop:
 *
 *     - the frame prepare sets the voice count `0x74458` and the index
 *       `0x7445c` starts at zero
 *     - the event helper at `0x8a8d8` returns a non-zero value when an event
 *       exists, and the event path then sets `0x694ec`, `0x7b804`, and
 *       `0x7b814` and runs the three event helpers
 *     - the frame index advances and the voice count decrements until zero
 *
 * The original sets DS and ES from the selector at `CS:0x214d4`.  The rebuild
 * omits the segment loads, because the flat target keeps one data segment and
 * the load has no effect on the data addresses.
 */
extern int sound_effect_handle;
extern int sound_active_count;
extern int sound_voice_count;
extern int sound_frame_repeat;
extern int sound_voice_index;
extern int sound_voice_remaining;
extern void *sound_song_data;
extern int sound_sample_table;
extern int sound_volume_left;
extern int sound_volume_right;

extern void sound_timer_update(void);
extern void sound_sequence_prepare(void);
extern void sound_source_update(void);
extern void sound_mix_prepare(void);
extern void sound_voice_mix(void *table, int count);
extern void sound_frame_prepare(void);
extern int sound_event_next(void);
extern void sound_event_transform(void);
extern void sound_event_render(void);
extern void sound_event_convert(void *source);

void sound_effects_update(int driver)
{
    sound_effect_handle = driver;
    sound_timer_update();
    sound_sequence_prepare();
    sound_source_update();
    if (sound_active_count <= 0) {
        return;
    }
    sound_mix_prepare();
    if (sound_voice_count <= 0) {
        return;
    }
    sound_voice_mix((void *)0x45790, sound_voice_count);
    sound_song_data = (void *)0x45790;
    sound_frame_repeat = sound_voice_count;
    while (sound_frame_repeat != 0) {
        sound_frame_prepare();
        sound_voice_index = 0;
        do {
            if (sound_event_next() != 0) {
                sound_event_transform();
                sound_sample_table = 0x694f0;
                sound_volume_left = 0x17700000;
                sound_volume_right = 0x17700000;
                sound_event_render();
                sound_event_convert((void *)0x4d3c8);
            }
            sound_voice_index++;
            sound_voice_remaining--;
        } while (sound_voice_remaining != 0);
        sound_frame_repeat--;
    }
}
