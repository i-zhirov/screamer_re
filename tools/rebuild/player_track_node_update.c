/* The rebuilt `player_track_node_update` module.
 *
 * The original sits at linear 0x1821c in STARTH.EXE and holds 220 instructions
 * and 756 bytes up to the `ret` at 0x1850f.  The record keeps the 780-byte
 * extent, which ends at the next function `player_car_reset_update` at 0x18528,
 * and sets no function size.
 *
 * The only direct caller is the second player step group at 0x15ab8, which
 * runs on the player object at object-3 offset 0xf91c.  The five view groups
 * call that group after the physics group at 0x15a80, so the module takes the
 * section The vehicle state chain.
 *
 * The function reads the 0x20-byte track nodes at the global table
 * `track_node_table` and the count `track_node_count`.  The node entry holds
 * the radius at `+0x00`, the value at `+0x04`, the surface grip at `+0x0c`,
 * the flags at `+0x10`, the x position at `+0x14`, and the y position at
 * `+0x1c`.
 *
 * The function examines the node before the current index, the current node,
 * and the next node.  It accepts a node when each absolute axis delta stays
 * below the node radius and the truncated distance does not exceed the radius.
 * It then sets the current index at `+0x598`, the target index at `+0x668`,
 * saves the old target value and flags to `+0x58c` and `+0x590`, and copies
 * the node value, the surface grip, and the flags to `+0x66c`, `+0x670`, and
 * `+0x678`.
 *
 * The visited chain at `+0x59c` and the second chain at `+0x5a0` select the
 * status `+0x780`: a hit on a visited node after the first, or a hit on a node
 * in the second chain, sets 2, and any other hit sets 1.  A call with no hit
 * sets the target index to -1.
 *
 * The function then forms the segment from the current node to the next node
 * and the distance from the object position at `+0x4b0` and `+0x4b8` to the
 * current node, both scaled by 0.05 and truncated.  It stores the ratio at
 * `+0x67c`.  The scale arrives through the global `track_node_scale`, because
 * a harness literal reads at a low address.
 *
 * The conversions to an integer use the Watcom helper at 0x4d9f4, which
 * truncates toward zero.  The module uses a local truncating FPU stub, because
 * the shared stub rounds to nearest.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int track_node_count;
extern double track_node_scale;
extern double sqrt(double value);

extern struct track_node {
    int radius;
    int value;
    int reserved08;
    float data;
    int flags;
    int x;
    int reserved18;
    int y;
} track_node_table[];

void player_track_node_update(void *object)
{
    int found;
    int pass;

    found = 0;
    for (pass = -1; pass < 2; pass++) {
        int index;
        int dx;
        int dy;
        struct track_node *entry;

        index = *(int *)((char *)object + 0x598) + pass;
        if (index >= track_node_count) {
            index -= track_node_count;
        }
        if (index < 0) {
            index += track_node_count;
        }
        entry = &track_node_table[index];
        dx = *(int *)((char *)object + 0x4b0) - entry->x;
        if (dx < 0) {
            dx = -dx;
        }
        if (dx >= entry->radius) {
            continue;
        }
        dy = *(int *)((char *)object + 0x4b8) - entry->y;
        if (dy < 0) {
            dy = -dy;
        }
        if (dy >= entry->radius) {
            continue;
        }
        dx = *(int *)((char *)object + 0x4b0) - entry->x;
        dy = *(int *)((char *)object + 0x4b8) - entry->y;
        if ((int)sqrt((double)(dx * dx + dy * dy)) > entry->radius) {
            continue;
        }
        found = 1;
        *(int *)((char *)object + 0x598) = index;
        *(int *)((char *)object + 0x668) = index;
        entry = &track_node_table[*(int *)((char *)object + 0x668)];
        *(int *)((char *)object + 0x58c) = *(int *)((char *)object + 0x66c);
        *(int *)((char *)object + 0x590) = *(int *)((char *)object + 0x678);
        *(int *)((char *)object + 0x66c) = entry->value;
        *(float *)((char *)object + 0x670) = entry->data;
        *(int *)((char *)object + 0x678) = entry->flags;
        if (index != 0
            && *(int *)((char *)object + 0x59c + index * 4) != 0) {
            *(int *)((char *)object + 0x780) = 2;
            continue;
        }
        if (*(int *)((char *)object + 0x5a0 + index * 4) != 0) {
            *(int *)((char *)object + 0x780) = 2;
            continue;
        }
        *(int *)((char *)object + 0x780) = 1;
    }
    if (found == 0) {
        *(int *)((char *)object + 0x668) = -1;
    }
    {
        int index;
        int next;
        int dx;
        int dy;
        int segment;
        int distance;

        index = *(int *)((char *)object + 0x598);
        next = index + 1;
        if (next >= track_node_count) {
            next -= track_node_count;
        }
        dx = track_node_table[next].x - track_node_table[index].x;
        dy = track_node_table[next].y - track_node_table[index].y;
        segment = (int)sqrt(
            (double)dx * track_node_scale * ((double)dx * track_node_scale)
            + (double)dy * track_node_scale * ((double)dy * track_node_scale));
        dx = *(int *)((char *)object + 0x4b0) - track_node_table[index].x;
        dy = *(int *)((char *)object + 0x4b8) - track_node_table[index].y;
        distance = (int)sqrt(
            (double)dx * track_node_scale * ((double)dx * track_node_scale)
            + (double)dy * track_node_scale * ((double)dy * track_node_scale));
        *(float *)((char *)object + 0x67c) =
            (float)distance / (float)segment;
    }
}
