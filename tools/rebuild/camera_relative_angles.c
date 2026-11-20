/* The rebuilt `camera_relative_angles` module.
 *
 * The original sits at linear 0x4bee0 in STARTH.EXE and holds 26 instructions
 * and 67 bytes up to the `ret` at 0x4bf22.  The next code starts at 0x4bf23,
 * so the store record keeps the 67-byte extent and sets no separate function
 * size.  The identity check uses 26 instructions.
 *
 * The function takes the object in `eax` and the output triple in `edx`.  The
 * camera pointer is the first field of the screen record at the global pointer
 * `PTR_DAT_0008a844`, which `screen_mode_set` at 0x4c850 and `race_data_load`
 * at 0x131a8 also fill.  The camera record holds the three angle indices at
 * `+0x00`, `+0x04`, and `+0x08`, as `camera_project` at 0x82c14 shows.
 *
 * The object fields `+0x14` and `+0x18` hold the object angles.  The function
 * writes the relative yaw, the relative pitch with the quarter-turn bias, and
 * the inverse camera roll.  The yaw and the pitch wrap with the `0x7ff` mask.
 * The roll is the complement `0x7ff - angle2` without a mask.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses no floating-point instruction and calls no function, so it
 * needs no FPU stub and no assembly implementation.
 */
#define ANGLE_MASK 0x7ff
#define QUARTER_TURN 0x400

struct camera {
    int angle0;
    int angle1;
    int angle2;
};

struct screen_record {
    struct camera *camera;
};

struct object_record {
    char pad_00[0x14];
    int angle_14;
    int angle_18;
};

struct relative_angles {
    int yaw;
    int pitch;
    int roll;
};

extern struct screen_record *PTR_DAT_0008a844;

void camera_relative_angles(struct object_record *object,
    struct relative_angles *relative)
{
    struct camera *camera;

    camera = PTR_DAT_0008a844->camera;
    relative->roll = ANGLE_MASK - camera->angle2;
    relative->pitch =
        (camera->angle1 - object->angle_18 - QUARTER_TURN) & ANGLE_MASK;
    relative->yaw = (object->angle_14 - camera->angle0) & ANGLE_MASK;
}
