/* The rebuilt `sound_emitter_update` module.
 *
 * The original sits at linear 0x21b9c in STARTH.EXE and holds 153 instructions
 * and 524 bytes up to the `ret` at `0x21da7`.  The next function starts at
 * `0x21da8`, so the store record keeps the 524-byte extent and sets no
 * function size.
 *
 * The only direct caller at `0x20b0a` is the opponent update at `0x20954`,
 * which calls this function for each running opponent record.  The second
 * argument is 1 when the opponent update runs without its argument, so the
 * function updates the sound only on that path.
 *
 * The function forms the emitter distance from the camera position
 * `DAT_0013ccc` and `DAT_0013cd0` and the record position at `+0x20` and
 * `+0x28`.  It shifts each axis delta by 3, squares the two values, and takes
 * the square root through 0x22bec, scaled by 8.0.
 *
 * A clear sound at `+0x38` starts when the distance stays below 40000.0: the
 * function stores the truncated distance at `+0x3c` and gets the sound id from
 * the table through 0x1f718 with `DAT_0018d88` plus the record index at
 * `+0x14`.  A set sound stops through 0x1f880 when the distance rises above
 * 40000.0.  A sound that stays clear at that distance returns.
 *
 * A running sound copies the float at `+0xc` to `+0x40`, rotates the camera
 * delta by the camera angle `DAT_0013cd4` through 0x2218c, and forms three
 * values: a pitch from the stored distance, a squared volume falloff, and a
 * stereo pan from the rotated lateral value.  It stores the new truncated
 * distance at `+0x3c`, and it sets the pitch through 0x1f7e4, the volume
 * through 0xc667d, and the pan through 0xc6920.  The low bit of
 * `DAT_0011d261` and the flag `DAT_0010a8c` can suppress the three stores.
 *
 * The conversions to an integer use the Watcom helper at 0x4d9f4, which
 * truncates toward zero.  The shared `fpu_stub.asm` leaves round-to-nearest,
 * so the module uses a local truncating `__CHP` stub.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_00008058;
extern int DAT_0013ccc;
extern int DAT_0013cd0;
extern float DAT_0013cd4;
extern int DAT_0018d88;
extern unsigned int DAT_0018d94;
extern unsigned char DAT_0011d261;
extern int DAT_0010a8c;

extern double distance_scale;
extern float sound_range;
extern float pitch_scale;
extern float pitch_base;
extern float volume_scale;
extern double pan_scale;

extern double sqrt(double value);
extern double fabs(double value);
extern int table_lookup(int index);
extern int sound_stop(int id);
extern void vector_rotate(int *out, int *in, float angle);
extern void sound_set_pitch(int id, int value);
extern void sound_set_volume(unsigned int owner, int id, int value);
extern void sound_set_pan(unsigned int owner, int id, int value);

void sound_emitter_update(char *object, int enabled)
{
    int delta[3];
    int rotated[3];
    int dx;
    int dz;
    int distance2;
    int sign;
    int pitch;
    int volume;
    int pan;
    float distance;
    float ratio;

    if (DAT_00008058 == 0) {
        return;
    }
    if (enabled == 0) {
        return;
    }
    dx = (*(int *)(object + 0x20) - DAT_0013ccc) >> 3;
    dz = (*(int *)(object + 0x28) - DAT_0013cd0) >> 3;
    distance2 = dx * dx + dz * dz;
    distance = (float)(sqrt((double)distance2) * distance_scale);
    if (*(int *)(object + 0x38) != -1) {
        if (distance > sound_range) {
            *(int *)(object + 0x38) =
                sound_stop(*(int *)(object + 0x38));
        }
    } else if (distance < sound_range) {
        *(int *)(object + 0x3c) = (int)distance;
        *(int *)(object + 0x38) =
            table_lookup(DAT_0018d88 + *(int *)(object + 0x14));
    }
    if (*(int *)(object + 0x38) == -1) {
        return;
    }
    *(float *)(object + 0x40) = *(float *)(object + 0xc);
    delta[0] = DAT_0013ccc - *(int *)(object + 0x20);
    delta[2] = DAT_0013cd0 - *(int *)(object + 0x28);
    vector_rotate(rotated, delta, DAT_0013cd4);
    pitch = (int)(((float)*(int *)(object + 0x3c) - distance)
        * pitch_scale + pitch_base);
    ratio = (sound_range - distance) / sound_range;
    volume = (int)(ratio * ratio * volume_scale);
    if (rotated[0] >= 0) {
        sign = 1;
    } else {
        sign = -1;
    }
    pan = (int)(sqrt(fabs((double)rotated[0] / (double)distance))
        * pan_scale * (double)sign) + 0x8000;
    *(int *)(object + 0x3c) = (int)distance;
    if (!(DAT_0011d261 & 1) || DAT_0010a8c == 0) {
        sound_set_pitch(*(int *)(object + 0x38), pitch);
        sound_set_volume(DAT_0018d94, *(int *)(object + 0x38), volume);
        sound_set_pan(DAT_0018d94, *(int *)(object + 0x38), pan);
    }
}
