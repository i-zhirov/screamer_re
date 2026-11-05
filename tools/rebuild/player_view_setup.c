/* The rebuilt `player_view_setup` module.
 *
 * The original sits at linear 0x19ae8 in STARTH.EXE.  The first-return body
 * holds 223 instructions and 869 bytes up to the `ret` at 0x19e4c.  The store
 * keeps the 912-byte extent and sets no function size.
 *
 * The player update at 0x19ab0 runs this function as its first step on the
 * player structure `DAT_00110a20`.  The car camera at 0x1a930 also runs it on
 * the view block at `+0x4f8` of each 0x590-byte car record at `DAT_00110ab8`.
 *
 * The function selects the active view from the seven 0x14-byte slots at
 * `DAT_00101da4`:
 *
 *     slot +0x00   the view kind, or 9 for a special slot
 *     slot +0x04   the render mode
 *     slot +0x08   the view distance
 *     slot +0x0c   the video mode
 *     slot +0x10   the chosen flag
 *
 * A slot is eligible when the input bits `DAT_00118e1c` and `DAT_00118e1d`
 * permit the change, the byte `DAT_000f003d[kind]` is set, and the slot is not
 * the current slot.  A slot with the chosen flag set is also eligible.  The
 * function copies the slot distance, the video mode, and the render mode into
 * the view, sets the screen mode, copies the car position and angle into the
 * view, and starts the renderer or the car camera.  It marks the slot chosen
 * flag clear and records the first free slot in `DAT_0011d2a8`.
 *
 * The special slot 4 with the chosen flag runs the camera scan over the
 * `DAT_00110a1c` counter and the `DAT_0011100c` field of the car records.
 *
 * The original converts the view distance with the Watcom helper at 0x4d9f4,
 * which truncates toward zero.  The module uses a local truncating FPU stub,
 * because the shared stub rounds to nearest.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
struct view_slot {
    int kind;
    int mode;
    int distance;
    int video;
    int chosen;
};

struct player_view {
    int x;
    int y;
    int z;
    int field_0c;
    int field_10;
    int field_14;
    int copy_x;
    int copy_y;
    int copy_z;
    int field_24;
    int field_28;
    int field_2c;
    float angle_x;
    float angle_y;
    float angle_z;
    int field_3c;
    int field_40;
    int field_44;
    float saved_x;
    float saved_y;
    float saved_z;
    int flag_54;
    int flag_58;
    int field_5c;
    int field_60;
    int field_64;
    int field_68;
    int flag_6c;
    int field_70;
    int flag_74;
    int field_78;
    int flag_7c;
    int flag_80;
    int index;
    int mode;
    int video;
    int distance;
    int car;
};

extern struct view_slot DAT_00101da4[7];
extern unsigned char DAT_000f003d[];
extern unsigned char DAT_00118e1c;
extern unsigned char DAT_00118e1d;
extern int DAT_00118e6c;
extern unsigned char DAT_0011d274;
extern int DAT_001100c0;
extern int DAT_0011d2a8;
extern int DAT_0010f8ec;
extern int DAT_00110a18;
extern int DAT_00110a1c;
extern int DAT_0011100c[];
extern unsigned char DAT_00110ab8[];
extern double view_distance_scale;

extern void screen_mode_set(unsigned int width, unsigned int height,
                            unsigned int value);
extern void view_render(struct player_view *view);
extern void car_camera(void *car);

void player_view_setup(struct player_view *view)
{
    struct view_slot *slot;
    int i;
    int j;
    int rounded;

    view->flag_6c = 0;
    view->flag_80 = view->flag_6c;

    i = 0;
    while (i <= 6) {
        slot = &DAT_00101da4[i];
        if ((slot->kind != 9
             && DAT_000f003d[slot->kind] != 0
             && (DAT_00118e1d & 0x20) == 0
             && DAT_00118e6c == 0
             && view->flag_58 == 0
             && (DAT_00118e1c & 3) != 0
             && i != view->index)
            || slot->chosen != 0) {
            break;
        }
        ++i;
    }
    if (i > 6) {
        return;
    }
    if (i == 1 && (DAT_0011d274 & 0x20) != 0) {
        DAT_00101da4[1].chosen = 0;
        i = 0;
    }

    view->distance = DAT_00101da4[i].distance;
    view->video = DAT_00101da4[i].video;
    if (view->flag_7c == 0) {
        screen_mode_set(0x140, 0x190, 0x104);
        if (DAT_001100c0 == 1) {
            DAT_001100c0 = 2;
        }
        view->flag_54 = 1;
        view->flag_58 = view->flag_54;
        if (view->flag_74 == 0) {
            if (i == 4) {
                car_camera(DAT_00110ab8 + view->car * 0x590);
            } else {
                view_render(view);
            }
        }
        view->copy_x = view->x;
        view->copy_y = view->y;
        view->copy_z = view->z;
        view->saved_x = view->angle_x;
        view->saved_y = view->angle_y;
        view->saved_z = view->angle_z;
    } else {
        rounded = (int)(view->distance * view_distance_scale);
        screen_mode_set(0x140, view->distance, rounded);
        DAT_001100c0 = DAT_00101da4[i].video;
    }

    view->mode = DAT_00101da4[i].mode;
    view->flag_6c = 1;
    view->index = i;
    if (DAT_00101da4[i].chosen == 0) {
        DAT_0011d2a8 = i;
    }
    view->flag_80 = 1;
    if (DAT_00101da4[view->index].chosen == 1 && view->index == 4) {
        view->flag_7c = 1;
        for (j = 0; j < DAT_0010f8ec; ++j) {
            DAT_00110a1c = DAT_00110a1c + 1;
            if (DAT_00110a1c == DAT_0010f8ec) {
                DAT_00110a1c = 0;
            }
            if (DAT_00110a18 == DAT_00110a1c) {
                break;
            }
            if (DAT_0011100c[DAT_00110a1c * 0x164]
                != DAT_0011100c[DAT_00110a18 * 0x164]) {
                break;
            }
        }
    }
    DAT_00101da4[view->index].chosen = 0;
    if (view->flag_74 != 0) {
        car_camera(DAT_00110ab8 + view->car * 0x590);
    }
}
