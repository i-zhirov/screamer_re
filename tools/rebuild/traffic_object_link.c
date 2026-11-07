/* The rebuilt `traffic_object_link` module.
 *
 * The original sits at linear 0x1ef68 in STARTH.EXE and holds 117 instructions
 * and 415 bytes up to the `ret` at 0x1f106.  The next function starts at
 * 0x1f108, so the store record keeps the 416-byte extent and sets no function
 * size.  The identity check uses 117 instructions.
 *
 * The only direct caller at 0x202b1 is the traffic view group at 0x201d0.
 * That group walks the 0x2a8-byte traffic records at `DAT_00113cd8` while the
 * index stays below the count `DAT_00115218`.  The group first runs the record
 * update at 0x1f108 and then this function, so the module takes the section
 * `The vehicle state chain`.
 *
 * A negative first record field ends the function at once, inside the frame.
 *
 * The record field `+0x64` selects the 28-byte entry at `DAT_0010653c`.  The
 * entry field 0 is the body object and the entry field 4 is the render array.
 * The record field `+0x6c` selects the second 28-byte entry; its field 0 is
 * the part object.  The original loads the render pointer and stores it in a
 * local, but the body never reads that local again.
 *
 * The function copies the record position `+0x3c`, `+0x40`, and `+0x44` to the
 * body offsets 8, 0xc, and 0x10.  The position `+0x3c` and `+0x44` also go to
 * the part offsets 8 and 0x10.  The record field `+0x18` goes to the body
 * offset 0x3c, and the record field `+0x14` masked with 0xf000 goes to the body
 * offset 0x20.
 *
 * The function scales the record floats `+0x48`, `+0x4c`, `+0x50`, `+0x58`, and
 * `+0x5c` by 1024.0 and divides by pi.  It truncates each result toward zero
 * and masks the low 11 bits.  The values go to the body offsets 0x14, 0x18, and
 * 0x1c and to the part offsets 0x14 and 0x1c.  The body value at 0x18 also goes
 * to the part offset 0x18.  The record field `+0x54` goes to the part offset
 * 0xc.
 *
 * The traffic table, the entry table, and the sibling `object_car_link` support
 * the name `traffic_object_link`.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The five conversions to an integer use the Watcom helper at 0x4d9f4, which
 * truncates toward zero.  The module uses a local truncating FPU stub, because
 * the shared stub rounds to nearest.  The constants arrive through globals,
 * because the minimal image does not load the constant segment.  The original
 * has the EBP frame, so the module builds with `-of+`.
 */
extern unsigned char DAT_0010653c[];
extern float DAT_00100bc4;
extern float DAT_00100bc8;

void traffic_object_link(char *record)
{
    char *body;
    char *part;
    char *volatile render;

    if (*(int *)record < 0) {
        return;
    }
    render = *(char **)(DAT_0010653c + *(int *)(record + 0x64) * 28 + 4);
    body = *(char **)(DAT_0010653c + *(int *)(record + 0x64) * 28);
    part = *(char **)(DAT_0010653c + *(int *)(record + 0x6c) * 28);

    *(int *)(body + 8) = *(int *)(record + 0x3c);
    *(int *)(part + 8) = *(int *)(body + 8);
    *(int *)(body + 0xc) = *(int *)(record + 0x40);
    *(int *)(body + 0x10) = *(int *)(record + 0x44);
    *(int *)(part + 0x10) = *(int *)(body + 0x10);
    *(int *)(body + 0x14) = (int)(*(float *)(record + 0x48)
        * DAT_00100bc4 / DAT_00100bc8) & 0x7ff;
    *(int *)(body + 0x18) = (int)(*(float *)(record + 0x4c)
        * DAT_00100bc4 / DAT_00100bc8) & 0x7ff;
    *(int *)(part + 0x18) = *(int *)(body + 0x18);
    *(int *)(body + 0x1c) = (int)(*(float *)(record + 0x50)
        * DAT_00100bc4 / DAT_00100bc8) & 0x7ff;
    *(int *)(body + 0x3c) = *(int *)(record + 0x18);
    *(int *)(body + 0x20) = *(int *)(record + 0x14) & 0xf000;
    *(int *)(part + 0xc) = *(int *)(record + 0x54);
    *(int *)(part + 0x14) = (int)(*(float *)(record + 0x58)
        * DAT_00100bc4 / DAT_00100bc8) & 0x7ff;
    *(int *)(part + 0x1c) = (int)(*(float *)(record + 0x5c)
        * DAT_00100bc4 / DAT_00100bc8) & 0x7ff;
}
