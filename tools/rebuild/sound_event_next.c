/* The rebuilt `sound_event_next` module.
 *
 * The original body at 0x8a8d8 holds 12 instructions and 45 bytes up to the
 * `ret` at 0x8a904.  A branch at 0x8a902 enters the second block at 0x8a908,
 * and that block ends with the `ret` at 0x8a952.  The next recorded function
 * starts at 0x8a958, so the store record keeps the 128-byte extent and sets no
 * separate function size.  The identity check uses 12 instructions.
 *
 * The only direct caller is the inner loop of `sound_effects_update` at
 * 0x81ab0.  The caller tests the returned value and runs the event helpers when
 * the value is non-zero.  The event path runs in the shared sound service body,
 * so the record takes the section `object_update_all callees`.
 *
 * The context record at 0x74454 holds the frame table base at +0x00, the voice
 * count at +0x04, the frame index at +0x08, the view record at +0x0c, and the
 * event record at +0x10.  The table at 0x7a6e4 holds one 4-byte entry for each
 * frame index: the signed short voice slot and a second short value.  The voice
 * record size is 0x1f4, and the voice record array starts at 0x7453c.
 *
 * The function takes the voice slot of the current frame index, forms the voice
 * record pointer, and stores it in the view record field.  It reads the first
 * dword of the voice record, stores it as the event record pointer, and returns
 * 0 when the pointer is null.
 *
 * For a non-null event the function runs the setup helper at 0x83560 on the
 * camera basis at voice + 0x118.  It then copies the event flag at +0x18 to
 * 0x74530.  The event offsets at +0x1c, +0x20, and +0x24 are relative to the
 * frame table base, so the function adds the base and stores the three source
 * pointers at 0x7446c, 0x74470, and 0x74474.  It returns 1.
 *
 * The original has no EBP frame and uses no floating-point instruction, so the
 * module builds without `-of+` and links no FPU stub.  The module uses no
 * assembly implementation.
 */
#define VOICE_RECORD_SIZE 0x1f4
#define VOICE_BASIS_OFFSET 0x118
#define EVENT_HEADER_SIZE 0x18

struct source_distance {
    short id;
    short distance;
};

struct voice_context {
    char *frame_table;
    int voice_count;
    int frame_index;
    char *view_record;
    char *event_record;
};

struct sound_event {
    char header[EVENT_HEADER_SIZE];
    int render_flag;
    int source_offset;
    int point_offset;
    int screen_offset;
};

extern struct voice_context voice_context;
extern struct source_distance source_distances[];
extern char *voice_records;
extern int event_render_flag;
extern char *event_source;
extern char *event_point;
extern char *event_screen;

extern void sub_83560(char *basis);

int sound_event_next(void)
{
    struct sound_event *event;
    char *voice;
    int index;

    index = voice_context.frame_index;
    voice = voice_records + source_distances[index].id * VOICE_RECORD_SIZE;
    voice_context.view_record = voice;
    voice_context.event_record = *(char **)voice;
    if (voice_context.event_record == 0) {
        return 0;
    }

    sub_83560(voice + VOICE_BASIS_OFFSET);

    event = (struct sound_event *)voice_context.event_record;
    event_render_flag = event->render_flag;
    event_source = voice_context.frame_table + event->source_offset;
    event_point = voice_context.frame_table + event->point_offset;
    event_screen = voice_context.frame_table + event->screen_offset;
    return 1;
}
