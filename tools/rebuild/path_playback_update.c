/* The rebuilt `path_playback_update` module.
 *
 * The original sits at linear 0x23038 in STARTH.EXE and holds 148
 * instructions and 437 bytes up to the `ret` at 0x231ec.  The next function
 * starts at 0x231f0, so the store record keeps the 440-byte extent and sets no
 * function size.  The identity check uses 148 instructions.
 *
 * The only direct caller at 0x20317 is the view group at 0x202c0.  That group
 * appends a keyframe through 0x231f0 and then calls this function with the
 * playback block at object-3 offset 0x18ed8 in `eax` and zero in `edx`.  The
 * frame update at 0x1ff70 selects the view group through the mode byte.
 *
 * The first argument points to a playback block.  The field at `+0x08` points
 * to a state record with the index at `+0x04`, the count at `+0x08`, the
 * active flag at `+0x0c`, and the keyframe array at `+0x20`.  Each keyframe
 * holds 0x28 bytes:
 *
 *     +0x00 x, +0x04 y, +0x08 z          (integers)
 *     +0x0c, +0x10, +0x14                (angles as floats)
 *     +0x18, +0x1c                       (two integer fields)
 *     +0x20                              (a float field)
 *     +0x24                              (the timestamp as a float)
 *
 * The function returns at once when the global `DAT_0011d264` is zero or when
 * the state is not active (the flag at `+0x0c` is not 1).  When the second
 * argument is zero, it advances the state index while the playback time
 * `DAT_00118e10` is above the timestamp of the current keyframe.  When the
 * index reaches the count, it sets the active flag to -1.
 *
 * The function then takes the keyframe before the index (the head) as the
 * current keyframe and copies its position, angles, and other fields to the
 * playback block at `+0x10` through `+0x30`.  The ternary keeps the head at
 * zero when the index is zero.  The second argument also sets the head to zero
 * and stops after the copy.
 *
 * On the normal path the function forms the fraction between the head stamp
 * and the next stamp, takes the keyframes before, at, after, and two after the
 * head, and calls `spline_interpolate` with the fraction.  It calls the spline
 * only when the head is above one and the count minus the head is above two,
 * because the cubic form needs the two neighbours.
 *
 * The original reads the global playback time at object-3 offset 0x18e10 and
 * the global enable flag at object-3 offset 0x1d264.  The two globals arrive
 * from the harness, because the minimal image does not load that part of the
 * segment.  The original has the EBP frame, so the module builds with `-of+`.
 */
extern float DAT_00118e10;
extern int DAT_0011d264;

void spline_interpolate(int **frames, int *out, float t);

void path_playback_update(char *out, int flag)
{
    char *state;
    char *frames;
    char *head;
    int index;
    int head_index;
    int *pointers[4];
    float head_stamp;
    float next_stamp;
    float fraction;

    if (DAT_0011d264 == 0) {
        return;
    }
    state = *(char **)(out + 0x08);
    if (*(int *)(state + 0x0c) != 1) {
        return;
    }

    if (flag == 0) {
        for (;;) {
            state = *(char **)(out + 0x08);
            index = *(int *)(state + 0x04);
            frames = *(char **)(state + 0x20);
            if (!(DAT_00118e10 > *(float *)(frames + index * 0x28 + 0x24))) {
                break;
            }
            index += 1;
            *(int *)(state + 0x04) = index;
            state = *(char **)(out + 0x08);
            if (index != *(int *)(state + 0x08)) {
                continue;
            }
            *(int *)(state + 0x0c) = -1;
            break;
        }
        state = *(char **)(out + 0x08);
        index = *(int *)(state + 0x04);
        head_index = index != 0 ? index - 1 : 0;
    } else {
        head_index = 0;
    }

    frames = *(char **)(state + 0x20);
    head = frames + head_index * 0x28;

    *(int *)(out + 0x10) = *(int *)(head + 0x00);
    *(int *)(out + 0x14) = *(int *)(head + 0x04);
    *(int *)(out + 0x18) = *(int *)(head + 0x08);
    *(float *)(out + 0x1c) = *(float *)(head + 0x0c);
    *(float *)(out + 0x20) = *(float *)(head + 0x10);
    *(float *)(out + 0x24) = *(float *)(head + 0x14);
    *(int *)(out + 0x28) = *(int *)(head + 0x18);
    *(float *)(out + 0x30) = *(float *)(head + 0x20);
    *(int *)(out + 0x2c) = *(int *)(head + 0x1c);

    if (flag != 0) {
        return;
    }

    head_stamp = *(float *)(head + 0x24);
    index = *(int *)(state + 0x04);
    next_stamp = *(float *)(frames + index * 0x28 + 0x24);

    if (head_index <= 1) {
        return;
    }
    if (*(int *)(state + 0x08) - head_index <= 2) {
        return;
    }

    fraction = (DAT_00118e10 - head_stamp) / (next_stamp - head_stamp);

    pointers[0] = (int *)(head - 0x28);
    pointers[1] = (int *)head;
    pointers[2] = (int *)(head + 0x28);
    pointers[3] = (int *)(head + 0x50);

    spline_interpolate(pointers, (int *)(out + 0x10), fraction);
}
