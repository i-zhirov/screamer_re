/* The rebuilt `player_segment_update` module.
 *
 * The original sits at linear 0x11f98 in STARTH.EXE and holds 96 instructions
 * and 295 bytes up to the `ret` at 0x120be.  The next function starts at
 * 0x120c0, so the store record keeps the 296-byte extent and sets no separate
 * function size.  The identity check uses 96 instructions.
 *
 * The only direct caller at 0x15adc is the second player step group at
 * 0x15ab8.  The group runs the player car steps on the player object at
 * object-3 offset 0xf91c after `player_lap_progress_update` and after the
 * detection helper at 0x11e74, and before `wheel_contact_update` at 0x11a30,
 * so the module takes the section The vehicle state chain.
 *
 * The detection helper at 0x11e74 sets the flag `+0x40` when a body point
 * meets a collision shape and stores one clear flag at `+0x18` of each of the
 * four 0x2c-byte slots at `+0x54` through `+0x104`.  This function runs only
 * when the flag `+0x40` is set and skips a slot with a clear flag `+0x18`.
 *
 * For each active slot the function publishes the slot position `+0x0c` and
 * `+0x14` through `DAT_00108038` and `DAT_00108040` and asks the nearest
 * segment search at 0x120c0 in the segment mode.  On a hit the search leaves
 * the nearest segment at `DAT_00108044`; the segment holds the angle at
 * `+0x00`, the x position at `+0x04`, and the y position at `+0x08`.
 *
 * The function forms the delta from the segment position to the slot, turns
 * the delta by the negated segment angle through 0x2218c, and replaces the
 * local x term with plus or minus 30 according to its sign.  It turns the
 * result by the segment angle again and stores the difference from the delta
 * at `+0x1c` and `+0x24`, and the segment angle at `+0x28`.  The contact step
 * at 0x16b98 reads these fields and shifts the player position by the
 * smallest lateral term.
 *
 * A miss clears `+0x1c`, `+0x24`, and `+0x28`.  The rotation helper writes two
 * int values eight bytes apart, so the module uses the point structure below.
 * The original has the EBP frame, so the module builds with `-of+`.
 */
struct point3 {
    int x;
    int y;
    int z;
};

extern int DAT_00108038;
extern int DAT_00108040;
extern char *DAT_00108044;

extern int track_nearest_node(int mode);
extern void rotate_point(struct point3 *out, struct point3 *in, float angle);

void player_segment_update(char *self)
{
    char *slot;
    char *end;
    char *segment;
    struct point3 delta;
    struct point3 local;
    struct point3 pushed;

    if (*(int *)(self + 0x40) == 0) {
        return;
    }
    slot = self + 0x54;
    end = self + 0x104;
    while (slot < end) {
        if (*(int *)(slot + 0x18) != 0) {
            DAT_00108038 = *(int *)(slot + 0x0c);
            DAT_00108040 = *(int *)(slot + 0x14);
            if (track_nearest_node(1) != 0) {
                segment = DAT_00108044;
                delta.x = DAT_00108038 - *(int *)(segment + 4);
                delta.z = DAT_00108040 - *(int *)(segment + 8);
                rotate_point(&local, &delta, -*(float *)segment);
                local.x = (local.x < 0 ? 1 : -1) * 30;
                rotate_point(&pushed, &local, *(float *)segment);
                *(int *)(slot + 0x1c) = pushed.x - delta.x;
                *(int *)(slot + 0x24) = pushed.z - delta.z;
                *(float *)(slot + 0x28) = *(float *)segment;
            }
        } else {
            *(int *)(slot + 0x24) = 0;
            *(int *)(slot + 0x1c) = *(int *)(slot + 0x24);
            *(int *)(slot + 0x28) = 0;
        }
        slot = slot + 0x2c;
    }
}
