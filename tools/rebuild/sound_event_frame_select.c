/* The rebuilt `sound_event_frame_select` module.
 *
 * The original sits at linear 0x8a848 in STARTH.EXE and holds 40 instructions
 * and 89 bytes up to the `ret` at 0x8a8a0.  The next store record starts at
 * 0x8a8d8, so the store record keeps the 144-byte extent and sets no separate
 * function size.  The identity check uses 40 instructions.
 *
 * The only direct caller is `sound_event_render` at 0x836ac, with the call site
 * at 0x8378b.  That function runs in the per-frame event path of
 * `sound_effects_update` at 0x81ab0, so the record takes the section
 * object_update_all callees.
 *
 * The caller puts the three scaled direction values in eax, ebx, and ecx.  It
 * reads the returned frame list pointer from esi and ignores the returned eax.
 * C cannot return a value in esi, so the rebuild returns the pointer in eax and
 * the harness compares that value.  The original also stores the direction
 * index at 0x7b7d0, so the rebuild stores it in the global
 * `frame_select_index`.
 *
 * The first value is the x axis, the second value is the y axis, and the third
 * value is the z axis.  The function forms a 16-way direction index:
 *
 *     base   0 when y is zero or positive, 8 when y is negative
 *     sector 0  x >= 0, z >= 0, z >  x
 *     sector 1  x >= 0, z >= 0, z <= x
 *     sector 2  x >= 0, z <  0, -z <= x
 *     sector 3  x >= 0, z <  0, -z >  x
 *     sector 4  x <  0, z <  0, -z >  -x
 *     sector 5  x <  0, z <  0, -z <= -x
 *     sector 6  x <  0, z >= 0, z <= -x
 *     sector 7  x <  0, z >= 0, z >  -x
 *
 * The index selects one dword from the 16-entry frame offset table at +0x28 of
 * the event record.  The event record pointer is the global 0x74464.  The
 * function returns the record base plus the selected offset, which is the frame
 * vertex list that `sound_event_render` walks in its far tail.
 *
 * The original has no EBP frame and uses no floating-point instruction, so the
 * module builds without `-of+` and links no FPU stub.  The module uses no
 * assembly implementation.
 */

#define EVENT_OFFSET_TABLE 0x28
#define EVENT_OFFSET_COUNT 16

struct sound_event {
    char header[EVENT_OFFSET_TABLE];
    int frame_offset[EVENT_OFFSET_COUNT];
};

extern struct sound_event *sound_event_record;
extern int frame_select_index;

short *sound_event_frame_select(int x, int y, int z)
{
    struct sound_event *event;
    int index;
    int magnitude_x;
    int magnitude_z;

    index = 0;
    if (y < 0) {
        index = 8;
    }
    if (x >= 0) {
        if (z >= 0) {
            if (z <= x) {
                index++;
            }
        } else {
            magnitude_z = -z;
            index += 2;
            if (magnitude_z > x) {
                index++;
            }
        }
    } else {
        magnitude_x = -x;
        if (z >= 0) {
            index += 6;
            if (z > magnitude_x) {
                index++;
            }
        } else {
            magnitude_z = -z;
            index += 4;
            if (magnitude_z <= magnitude_x) {
                index++;
            }
        }
    }
    event = sound_event_record;
    frame_select_index = index;
    return (short *)((char *)event + event->frame_offset[index]);
}
