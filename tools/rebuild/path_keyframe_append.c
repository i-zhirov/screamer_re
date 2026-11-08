/* The rebuilt `path_keyframe_append` module.
 *
 * The original sits at linear 0x231f0 in STARTH.EXE and holds 89 instructions
 * and 309 bytes up to the `ret` at 0x23324.  The next function starts at
 * 0x23328, so the store record keeps the 312-byte extent and sets no separate
 * function size.  The identity check uses 89 instructions.
 *
 * The only direct caller at 0x20306 is the view group at 0x202c0.  That group
 * passes the player object at object-3 offset 0xf91c in `eax` and a record at
 * object-3 offset 0x18e8c plus the view index times 0x24 in `edx`.  The group
 * then calls `path_playback_update` at 0x23038 with the playback block at
 * object-3 offset 0x18ed8.  The frame update at 0x1ff70 selects the view group
 * through the mode byte.
 *
 * The second argument points to a recording state with the count at `+0x00`,
 * the index at `+0x04`, the current index at `+0x08`, the active flag at
 * `+0x0c`, and the keyframe array at `+0x20`.  The function returns at once
 * when the active flag is not 1.
 *
 * The function writes the keyframe at `array + index * 0x28`.  Each keyframe
 * holds the same fields as the playback keyframe:
 *
 *     +0x00 x, +0x04 y, +0x08 z          (view base plus view offset)
 *     +0x0c, +0x10, +0x14                (three angle sums as floats)
 *     +0x18, +0x1c                       (two integer fields)
 *     +0x20                              (a float field)
 *     +0x24                              (the playback clock as a float)
 *
 * The position terms come from the fields at `+0x4b0` through `+0x4b8` plus
 * the fields at `+0x130` through `+0x138`.  The angle terms come from the
 * fields at `+0x4c8` through `+0x4d0` plus the fields at `+0x124` through
 * `+0x12c`.  The timestamp is the global clock at `DAT_00118e10`.  The other
 * fields come from `+0x7b4`, `+0x680`, and `+0x7b0`.
 *
 * The function then advances the index.  When the new index equals the count,
 * it keeps the index at the last slot, clears the active flag, and writes the
 * current index to `+0x08`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * uses floating-point instructions, so the recipe adds the FPU run-time stubs.
 */
extern float DAT_00118e10;

void path_keyframe_append(char *view, char *state)
{
    char *frame;
    int index;

    if (*(int *)(state + 0x0c) != 1) {
        return;
    }

    index = *(int *)(state + 0x04);
    frame = *(char **)(state + 0x20) + index * 0x28;

    *(int *)(frame + 0x00) = *(int *)(view + 0x4b0)
        + *(int *)(view + 0x130);
    *(int *)(frame + 0x04) = *(int *)(view + 0x4b4)
        + *(int *)(view + 0x134);
    *(int *)(frame + 0x08) = *(int *)(view + 0x4b8)
        + *(int *)(view + 0x138);
    *(float *)(frame + 0x0c) = *(float *)(view + 0x4c8)
        + *(float *)(view + 0x124);
    *(float *)(frame + 0x10) = *(float *)(view + 0x4cc)
        + *(float *)(view + 0x128);
    *(float *)(frame + 0x14) = *(float *)(view + 0x4d0)
        + *(float *)(view + 0x12c);
    *(float *)(frame + 0x24) = DAT_00118e10;
    *(int *)(frame + 0x18) = *(int *)(view + 0x7b4);
    *(float *)(frame + 0x20) = *(float *)(view + 0x680);
    *(int *)(frame + 0x1c) = *(int *)(view + 0x7b0);

    index += 1;
    *(int *)(state + 0x04) = index;
    if (index == *(int *)(state + 0x00)) {
        index -= 1;
        *(int *)(state + 0x04) = index;
        *(int *)(state + 0x0c) = 0;
    }
    *(int *)(state + 0x08) = index;
}
