/* The interim sound and music cluster.
 *
 * The whole-image link reports these symbols as undefined because the shipped
 * image holds the ADLIB driver, the music player, the sample mixer, the CD
 * audio path, and their state.  This module defines the sound and music entry
 * points with the calling convention of their callers and the state globals
 * that the rebuilt modules reference.
 *
 * The functions are a compatibility layer, not a byte-faithful recovery.  The
 * driver, mixer, and CD audio entry points are inert and return 0, and the
 * state starts at 0.  The module has no function record and no behavior check,
 * so it does not enter the verified count.
 */
struct mix_entry;
struct mix_source;
struct mix_voice;

float engine_sound_base;
float engine_sound_scale;
float sound_range;

int music_drum_data;
int music_drum_size;
int music_melodic_data;
int music_melodic_size;

int sound_active_count;
int sound_camera_x;
int sound_camera_y;
int sound_camera_z;
int sound_driver_handle;
int sound_effect_handle;
int sound_emitter_scale;
int sound_frame_repeat;
void **sound_handle_table;
int sound_handles[5];
int sound_mix_bound_x;
int sound_mix_bound_y;
int sound_mix_center_x;
int sound_mix_center_y;
struct mix_entry *sound_mix_entries;
int sound_mix_far_limit;
int *sound_mix_projection;
struct mix_source *sound_mix_sources;
struct mix_voice *sound_mix_voices;
int sound_sample_table;
void *sound_song_data;
char sound_state[0x24];
unsigned char sound_voice_context[0xdc];
int sound_voice_count;
unsigned char *sound_voice_entry;
int sound_voice_field;
int sound_voice_flag;
int sound_voice_index;
unsigned char sound_voice_records[0x40];
int sound_voice_remaining;
int sound_voice_shift;
int sound_voice_volume;
int sound_volume_left;
int sound_volume_right;

void cd_audio_poll(void)
{
}

void cd_audio_reset(void)
{
}

void cd_audio_stop(void)
{
}

void engine_sound_update(void *sound, int value)
{
    (void)sound;
    (void)value;
}

void engine_teardown(void)
{
}

int midi_song_load(void *buffer, char *record, int *table)
{
    (void)buffer;
    (void)record;
    (void)table;
    return 0;
}

void music_bank_stop(char *record)
{
    (void)record;
}

int music_ready(int track)
{
    (void)track;
    return 0;
}

void music_select(char *record, int value)
{
    (void)record;
    (void)value;
}

void music_step(void)
{
}

int sound_bank_install(int driver, int flag, void *data)
{
    (void)driver;
    (void)flag;
    (void)data;
    return 0;
}

void sound_basis_build(void *basis)
{
    (void)basis;
}

void sound_basis_build_event(void *basis, void *emitter)
{
    (void)basis;
    (void)emitter;
}

int sound_driver_load(int index, int *record, int *value, void *state,
                      int *bank)
{
    (void)index;
    (void)record;
    (void)value;
    (void)state;
    (void)bank;
    return 0;
}

void sound_enqueue(int id, int value, float extra)
{
    (void)id;
    (void)value;
    (void)extra;
}

void sound_event_convert(void *source)
{
    (void)source;
}

int sound_event_next(void)
{
    return 0;
}

void sound_event_transform(void)
{
}

void sound_frame_prepare(void)
{
}

void sound_marker(void)
{
}

void sound_reset(void)
{
}

void sound_sequence_prepare(void)
{
}

void sound_service_body(void *destination)
{
    (void)destination;
}

void sound_set_pan(unsigned int owner, int id, int value)
{
    (void)owner;
    (void)id;
    (void)value;
}

void sound_set_pitch(int id, int value)
{
    (void)id;
    (void)value;
}

void sound_set_volume(unsigned int owner, int id, int value)
{
    (void)owner;
    (void)id;
    (void)value;
}

void sound_slot_init(void)
{
}

void sound_slots_update(void)
{
}

void sound_source_update(void)
{
}

int sound_stop(int id)
{
    (void)id;
    return 0;
}

void sound_system_cleanup(void)
{
}

int sound_system_init(int a, int b, int c)
{
    (void)a;
    (void)b;
    (void)c;
    return 0;
}

void sound_timer_set(int value)
{
    (void)value;
}

void sound_timer_update(void)
{
}

void sound_voice_mix(void *table, int count)
{
    (void)table;
    (void)count;
}
