/* The rebuilt `object_car_link` module.
 *
 * The original sits at linear 0x1c84c in STARTH.EXE and holds 238 instructions
 * and 863 bytes up to the first `ret`: the three register saves, the frame, the
 * 20-byte local area, the two pointers from the 28-byte entry at
 * `DAT_0010653c`, the second pointer from the entry at `+4`, the position copy,
 * the five scaled and truncated values, the render array stores, the flags,
 * and the epilogue.
 *
 * The entry at `DAT_0010653c + index * 28` supplies the link pointer at field 0
 * and the render pointer at field 4.  The index comes from the object field
 * `+0x4dc`.  The object field `+4` selects a second entry; its field 0 is the
 * target pointer.
 *
 * The function copies the position at `+0x498`, `+0x49c`, and `+0x4a0` into
 * the link pointer at offsets 8, 0xc, and 0x10.  It scales the floats at
 * `+0x4a4`, `+0x4bc`, and `+0x4ac` by the constant 1024.0 and divides by pi.
 * It truncates each result toward zero and masks the low 11 bits.  The three
 * values go to link offsets 0x14, 0x18, and 0x1c.
 *
 * The floats at `+0x4b0` and `+0x4b8` form two more masked values.  The render
 * pointer indexes the four arrays at `+0x2dc`, `+0x2d4`, `+0x2cc`, and `+0x4e0`
 * with a 32-byte stride.  The first value goes to offset 8 of each array.  The
 * second value goes to offset 8 of the first array and to offset 0x10 of the
 * other three arrays.
 *
 * The render array at `+0x58` supplies an integer at offset 8.  The function
 * subtracts the angle at `+0x418` divided by 15.0, truncates the result toward
 * zero, masks the low 11 bits and clears bit 0.  The value goes to offset 8 of
 * the four arrays at `+0x58`, `+0x100`, `+0xac`, and `+0x154`.
 *
 * The target pointer receives the position at offsets 8 and 0x10, the float at
 * `+0x2c4` at offset 0xc, and the first masked value at offset 0x18.  When the
 * state at `+0x2f8` is 0, it also receives the values at 0x14 and 0x1c.  The
 * link pointer receives the value at `+0x4e8` at offset 0x3c.
 *
 * The link flags at offset 0x20 come from the table at `DAT_0010d268` indexed
 * by the type at `+0x2e0` with a 64-byte stride.  When the field `+0x2c0` is
 * 0, the function sets bit 0x4000.  When the angle at `+0x418` is at most
 * 4000.0, it sets bit 0x2000.
 *
 * The five conversions to an integer use the Watcom helper at 0x4d9f4, which
 * truncates toward zero.  The module uses a local truncating FPU stub, because
 * the shared stub rounds to nearest.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern unsigned char DAT_0010653c[];
extern float DAT_00100af4;
extern float DAT_00100af8;
extern float DAT_00100afc;
extern float DAT_00100b00;
extern unsigned char DAT_0010d268[];

void object_car_link(char *object)
{
    char *link;
    char *render;
    char *target;
    int value;

    link = *(char **)(DAT_0010653c + *(int *)(object + 0x4dc) * 28);
    render = *(char **)(DAT_0010653c + *(int *)(object + 0x4dc) * 28 + 4);
    target = *(char **)(DAT_0010653c + *(int *)(object + 4) * 28);

    *(int *)(link + 8) = *(int *)(object + 0x498);
    *(int *)(link + 0xc) = *(int *)(object + 0x49c);
    *(int *)(link + 0x10) = *(int *)(object + 0x4a0);
    *(int *)(link + 0x14) = (int)(*(float *)(object + 0x4a4)
        * DAT_00100af4 / DAT_00100af8) & 0x7ff;
    *(int *)(link + 0x18) = (int)(*(float *)(object + 0x4bc)
        * DAT_00100af4 / DAT_00100af8) & 0x7ff;
    *(int *)(link + 0x1c) = (int)(*(float *)(object + 0x4ac)
        * DAT_00100af4 / DAT_00100af8) & 0x7ff;

    value = (int)(*(float *)(object + 0x4b0)
        * DAT_00100af4 / DAT_00100af8) & 0x7ff;
    *(int *)(render + *(int *)(object + 0x2dc) * 32 + 8) = value;
    *(int *)(render + *(int *)(object + 0x2d4) * 32 + 8) = value;
    *(int *)(render + *(int *)(object + 0x2cc) * 32 + 8) = value;
    *(int *)(render + *(int *)(object + 0x4e0) * 32 + 8) = value;

    value = (int)(*(float *)(object + 0x4b8)
        * DAT_00100af4 / DAT_00100af8) & 0x7ff;
    *(int *)(render + *(int *)(object + 0x2dc) * 32 + 8) = value;
    *(int *)(render + *(int *)(object + 0x2d4) * 32 + 0x10) = value;
    *(int *)(render + *(int *)(object + 0x2cc) * 32 + 0x10) = value;
    *(int *)(render + *(int *)(object + 0x4e0) * 32 + 0x10) = value;

    value = *(int *)(render + *(int *)(object + 0x58) * 32 + 8);
    value = (int)((float)value - *(float *)(object + 0x418)
        / DAT_00100afc) & 0x7fe;
    *(int *)(render + *(int *)(object + 0x58) * 32 + 8) = value;
    *(int *)(render + *(int *)(object + 0x100) * 32 + 8) = value;
    *(int *)(render + *(int *)(object + 0xac) * 32 + 8) = value;
    *(int *)(render + *(int *)(object + 0x154) * 32 + 8) = value;

    *(int *)(target + 8) = *(int *)(object + 0x498);
    *(int *)(target + 0xc) = *(int *)(object + 0x2c4);
    *(int *)(target + 0x10) = *(int *)(object + 0x4a0);
    *(int *)(target + 0x18) = *(int *)(link + 0x18);
    if (*(int *)(object + 0x2f8) == 0) {
        *(int *)(target + 0x14) = *(int *)(link + 0x14);
        *(int *)(target + 0x1c) = *(int *)(link + 0x1c);
    }
    *(int *)(link + 0x3c) = *(int *)(object + 0x4e8);

    value = *(int *)(DAT_0010d268 + *(int *)(object + 0x2e0) * 64);
    if (*(int *)(object + 0x2c0) == 0) {
        value |= 0x4000;
    }
    if (!(*(float *)(object + 0x418) > DAT_00100b00)) {
        value |= 0x2000;
    }
    *(int *)(link + 0x20) = value;
}
