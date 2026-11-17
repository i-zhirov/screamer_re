/* The rebuilt `path_playback_apply` module.
 *
 * The original sits at linear 0x23328 in STARTH.EXE and holds 71 instructions
 * and 248 bytes up to the `ret` at 0x2341f.  The next function starts at
 * 0x23420, so the store record keeps the 248-byte extent and sets no separate
 * function size.  The identity check uses 71 instructions.
 *
 * The only direct caller at 0x20358 is the replay view group at 0x202c0.  That
 * group updates the playback block at object-3 offset 0x18ed8 through
 * `path_playback_update` at 0x23038 and then calls this function with the same
 * block.  The frame update at 0x1ff70 selects the view group through the mode
 * byte, so the module takes the section The vehicle state chain.
 *
 * The argument points to the playback block.  The field at `+0x00` selects the
 * 28-byte entry of the object table `DAT_0010653c`.  The entry field 0 is the
 * display object.  The field at `+0x08` points to the playback state, and the
 * flag at `+0x0c` of that state gates the write.
 *
 * The function returns at once when the global `DAT_0011d264` is zero or when
 * the state flag is zero.
 *
 * The function copies the sampled view from the playback block to the display
 * object:
 *
 *     +0x10, +0x14, +0x18   position to object +0x08, +0x0c, +0x10
 *     +0x1c, +0x20, +0x24   angles, scaled and masked, to +0x14, +0x18, +0x1c
 *     +0x28                 to object +0x20
 *     +0x2c                 to object +0x3c
 *
 * The three angles scale by 1024.0 and divide by pi.  The original calls the
 * Watcom helper at 0x4d9f4, which truncates toward zero, and masks the low 11
 * bits.  The constants arrive through globals, because the minimal image does
 * not load the constant segment.  The module uses a local truncating FPU stub,
 * because the shared stub rounds to nearest.  The original has the EBP frame,
 * so the module builds with `-of+`.
 */
extern unsigned char DAT_0010653c[];
extern int DAT_0011d264;
extern float DAT_00100dc8;
extern float DAT_00100dcc;

void path_playback_apply(char *view)
{
    char *state;
    char *object;

    if (DAT_0011d264 == 0) {
        return;
    }
    state = *(char **)(view + 8);
    if (*(int *)(state + 0xc) == 0) {
        return;
    }
    object = *(char **)(DAT_0010653c + *(int *)view * 28);
    *(int *)(object + 8) = *(int *)(view + 0x10);
    *(int *)(object + 0xc) = *(int *)(view + 0x14);
    *(int *)(object + 0x10) = *(int *)(view + 0x18);
    *(int *)(object + 0x14) = (int)(*(float *)(view + 0x1c)
        * DAT_00100dc8 / DAT_00100dcc) & 0x7ff;
    *(int *)(object + 0x18) = (int)(*(float *)(view + 0x20)
        * DAT_00100dc8 / DAT_00100dcc) & 0x7ff;
    *(int *)(object + 0x1c) = (int)(*(float *)(view + 0x24)
        * DAT_00100dc8 / DAT_00100dcc) & 0x7ff;
    *(int *)(object + 0x20) = *(int *)(view + 0x28);
    *(int *)(object + 0x3c) = *(int *)(view + 0x2c);
}
