/* The rebuilt `player_car_wheel_effect_update` module.
 *
 * The original sits at linear 0x19980 in STARTH.EXE and holds 90 instructions
 * and 297 bytes up to the `ret` at 0x19aa8.  The next function starts at
 * 0x19ab0, so the store record keeps the 304-byte extent and sets no separate
 * function size.  The identity check uses 90 instructions.
 *
 * The only direct caller at 0x15be0 is the effect group at 0x15bac.  The group
 * runs the player car steps on the player object at object-3 offset 0xf91c
 * after `player_car_spin_update` and before `car_sound_update`, and the five
 * view groups call the group, so the module takes the section The vehicle
 * state chain.
 *
 * The function walks the eight 0x28-byte effect slots at `+0x308` through
 * `+0x448`.  A slot with a clear flag at `+0x08` is skipped.  For each active
 * slot the function rotates the wheel offset at `+0x18` and `+0x20` by the
 * negated heading `+0x4cc` plus the yaw offset `+0x128`, rotates the result
 * again by the pitch `+0x4c8`, and stores the three position terms at `+0x0c`,
 * `+0x10`, and `+0x14`.
 *
 * The object index at `+0x00` selects one 0x1c-byte entry of the table
 * `DAT_0010653c`.  The object position `+0x08`, `+0x0c`, and `+0x10` takes the
 * slot position plus the player world position `+0x4b0`, `+0x4b4`, and
 * `+0x4b8`.
 *
 * The timer helper at 0x118e8 advances the object frame by the frame delta
 * DAT_00118e04 times the double rate below the constant segment.  A negative
 * result clears the slot flag and binds the list name at 0x6350 to the slot
 * through the helper at 0x11850, so the slot releases its object.
 *
 * The matching starter `player_car_wheel_effect_start` at 0x19828 fills these
 * slots, the caller group, and the effect slot layout support the name
 * player_car_wheel_effect_update.  The original has no symbol, so the record
 * keeps the name with the confidence inferred.
 *
 * The rotation helpers write two int values eight bytes apart, so the module
 * uses the point structure below.  The double rate arrives through a global,
 * because the minimal image does not load the constant segment.  The original
 * has the EBP frame, so the module builds with `-of+`.
 */
struct point3 {
    int x;
    int y;
    int z;
};

extern char DAT_0010653c[];
extern float DAT_00118e04;
extern double player_car_wheel_effect_rate;

extern void rotate_heading(struct point3 *out, struct point3 *in,
                           float angle);
extern void rotate_pitch(struct point3 *out, struct point3 *in, float angle);
extern int effect_slot_advance(void *table, void *slot, float delta);
extern int object_list_bind(void *table, char *name, void *slot);

void player_car_wheel_effect_update(char *self)
{
    char *slot;
    char *end;
    struct point3 point;
    char *object;

    slot = self + 0x308;
    end = self + 0x448;

    while (slot < end) {
        if (*(int *)(slot + 0x08) != 0) {
            rotate_heading(&point, (struct point3 *)(slot + 0x18),
                           -(*(float *)(self + 0x4cc)
                               + *(float *)(self + 0x128)));
            point.y = 0;
            rotate_pitch((struct point3 *)(slot + 0x0c), &point,
                         *(float *)(self + 0x4c8));

            object = *(char **)(DAT_0010653c + *(int *)slot * 28);
            *(int *)(object + 0x08) =
                *(int *)(slot + 0x0c) + *(int *)(self + 0x4b0);
            *(int *)(object + 0x0c) =
                *(int *)(slot + 0x10) + *(int *)(self + 0x4b4);
            *(int *)(object + 0x10) =
                *(int *)(slot + 0x14) + *(int *)(self + 0x4b8);

            if (effect_slot_advance((void *)0x6530, slot,
                                    DAT_00118e04
                                        * player_car_wheel_effect_rate) < 0) {
                *(int *)(slot + 0x08) = 0;
                object_list_bind((void *)0x6530, (char *)0x6350, slot);
            }
        }
        slot = slot + 0x28;
    }
}
