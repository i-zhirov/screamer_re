/* The rebuilt `joystick_update` module.
 *
 * The original sits at linear 0x24da0 in STARTH.EXE and holds 201 instructions
 * and 630 bytes: the six register saves, the frame with the 0x3c-byte local
 * area, the six-record walk over the 28-byte control records at `0x197d8`, the
 * duplicate search, the axis scaling with the FPU, the two raw button paths,
 * and the epilogue.
 *
 * The control record holds the type at `+0x00`, the kind at `+0x04`, the mode
 * at `+0x08`, the axis index at `+0x0c`, the device index at `+0x10`, and the
 * button mask or the button state pointer at `+0x18`.
 *
 * A record of type 1 and mode 0 or 2 is an axis.  A kind 1 record searches the
 * earlier records for the same device and the same axis, and copies the earlier
 * output when it finds one.  Otherwise the record reads the axis through
 * `0x10263`, divides the low 12 bits by the divisor at `+0x0c` of the 28-byte
 * axis record, subtracts the center at `+0x04`, multiplies by the scale at
 * `+0x14` or `+0x18`, truncates the product toward zero, applies the mode 2 and
 * full-scale clamps, and clears a value below 102 in magnitude.
 *
 * A kind other than 1 reads the port from the 10-dword table at `0x19704`
 * through `0x10257` and tests the mask at `+0x18`.  A mode other than 0 or 2
 * tests the byte at `+2` of the state pointer at `+0x18`.
 *
 * The output array sits at `0x19880`.  The axis table sits at `0x192a4` with a
 * 0x280-byte device stride and a 0x1c-byte axis stride.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * conversion to an integer uses the Watcom helper at `0x4d9f4`, which truncates
 * toward zero, so the module needs the truncating FPU stub.
 */
extern unsigned char DAT_001197d8[];
extern int DAT_0019880[];
extern unsigned char DAT_00192a4[];
extern int DAT_0019704[];
extern int joystick_axis_read(int mask, int port);
extern int joystick_button_read(int port);

struct control_record {
    int type;
    int kind;
    int mode;
    int axis;
    int device;
    int field_14;
    int field_18;
};

struct axis_record {
    int field_00;
    int center;
    int field_08;
    int divisor;
    int port;
    float scale_positive;
    float scale_negative;
};

void joystick_update(void)
{
    struct control_record *controls;
    struct axis_record *axis;
    int index;

    controls = (struct control_record *)DAT_001197d8;
    for (index = 0; index < 6; index++) {
        struct control_record *record;

        record = &controls[index];
        if (record->type != 1) {
            continue;
        }
        if (record->mode == 0 || record->mode == 2) {
            if (record->kind == 1) {
                int found;
                int earlier;

                found = 0;
                for (earlier = 0; earlier < index; earlier++) {
                    struct control_record *other;

                    other = &controls[earlier];
                    if (other->type == 1 && other->kind == 1
                        && (other->mode == 0 || other->mode == 2)
                        && other->device == record->device
                        && other->axis == record->axis) {
                        DAT_0019880[index] = DAT_0019880[earlier];
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    int raw;
                    int quotient;
                    int difference;
                    float scale;
                    int value;
                    int magnitude;

                    axis = (struct axis_record *)(DAT_00192a4
                        + record->device * 0x280 + record->axis * 0x1c);
                    raw = joystick_axis_read(1 << record->device, axis->port);
                    raw = raw & 0xfff;
                    quotient = raw / axis->divisor;
                    difference = quotient - axis->center;
                    if (difference > 0) {
                        scale = axis->scale_positive;
                    } else {
                        scale = axis->scale_negative;
                    }
                    value = (int)((float)difference * scale);
                    if (value > 0x3ff) {
                        value = 0x3ff;
                    }
                    if (record->mode == 2 && value > 0x1ff) {
                        value = 0x3ff;
                    }
                    if (value < -0x3ff) {
                        value = -0x3ff;
                    }
                    if (record->mode == 2 && value < -0x1ff) {
                        value = -0x3ff;
                    }
                    magnitude = value < 0 ? -value : value;
                    if (magnitude < 0x66) {
                        value = 0;
                    }
                    DAT_0019880[index] = value;
                }
            } else {
                int state;

                state = joystick_button_read(DAT_0019704[record->axis]);
                if ((state & record->field_18) != 0) {
                    DAT_0019880[index] = 0x3ff;
                } else {
                    DAT_0019880[index] = 0;
                }
            }
        } else {
            if (*(unsigned char *)(record->field_18 + 2) != 0) {
                DAT_0019880[index] = 0x3ff;
            } else {
                DAT_0019880[index] = 0;
            }
        }
    }
}
