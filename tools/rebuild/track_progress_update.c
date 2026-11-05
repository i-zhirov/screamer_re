/* The rebuilt `track_progress_update` module.
 *
 * The original sits at linear 0x1db94 in STARTH.EXE and holds 259 instructions
 * and 926 bytes: the six register saves, the frame, the 0x2c-byte local area,
 * the loop over the three neighbour nodes, the two radius guards and the
 * `sqrt` call at 0x22bec, the target stores, the visited chain, the lap reset,
 * the segment and object distance calls, the fraction, and the progress store.
 *
 * The function reads the track nodes at the global table `track_nodes`, with
 * the count `track_count`.  A node entry holds the radius at `+0x00`, the
 * target value at `+0x04`, the target data at `+0x08`, the x position at
 * `+0x14`, and the y position at `+0x1c`, on a 0x20-byte stride.
 *
 * The function examines the node before the current index, the current node,
 * and the next node.  It accepts a node when each absolute axis delta stays
 * below the node radius and the truncated distance does not exceed the radius.
 * It then advances the current index and the target index at `+0x320` and
 * `+0x3f0`, copies the node value and data to `+0x3f4` and `+0x3fc`, and marks
 * the node in the visited chain at `+0x324` and `+0x328`.  A node after the
 * first is marked only when the previous node is marked.  A visit to node 0
 * with every node marked clears the chain, sets the count to one, and
 * increments the lap counter at `+0x40c`.
 *
 * A call that finds no node sets the target index to -1.
 *
 * The function then forms the segment from the current node to the next node
 * and the distance from the object position at `+0x498` and `+0x4a0` to the
 * current node, both scaled by 0.05 and truncated.  It stores the ratio at
 * `+0x404` and the progress `100 * laps + index + ratio` at `+0x408`.  The
 * scale arrives through the global `track_scale`, because a harness literal
 * reads at a low address.
 *
 * The conversions to an integer use the Watcom helper at 0x4d9f4, which
 * truncates toward zero.  The module uses a local truncating FPU stub, because
 * the shared stub rounds to nearest.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int track_count;
extern struct track_node {
    int radius;
    int value;
    float data;
    int reserved0c;
    int reserved10;
    int x;
    int reserved18;
    int y;
} track_nodes[];

extern double track_scale;
extern double sqrt(double value);

void track_progress_update(void *object)
{
    int found;
    int pass;

    found = 0;
    for (pass = -1; pass < 2; pass++) {
        int index;
        int dx;
        int dy;
        struct track_node *entry;

        index = *(int *)((char *)object + 0x320) + pass;
        if (index >= track_count) {
            index -= track_count;
        }
        if (index < 0) {
            index += track_count;
        }
        entry = &track_nodes[index];
        dx = *(int *)((char *)object + 0x498) - entry->x;
        if (dx < 0) {
            dx = -dx;
        }
        if (dx >= entry->radius) {
            continue;
        }
        dy = *(int *)((char *)object + 0x4a0) - entry->y;
        if (dy < 0) {
            dy = -dy;
        }
        if (dy >= entry->radius) {
            continue;
        }
        dx = *(int *)((char *)object + 0x498) - entry->x;
        dy = *(int *)((char *)object + 0x4a0) - entry->y;
        if ((int)sqrt((double)(dx * dx + dy * dy)) > entry->radius) {
            continue;
        }
        found = 1;
        *(int *)((char *)object + 0x320) = index;
        *(int *)((char *)object + 0x3f0) = index;
        *(int *)((char *)object + 0x3f4) = track_nodes[index].value;
        *(float *)((char *)object + 0x3fc) = track_nodes[index].data;
        if (index != 0 &&
            *(int *)((char *)object + 0x324 + index * 4) == 0) {
            continue;
        }
        if (*(int *)((char *)object + 0x328 + index * 4) != 0) {
            int node;
            if (index != 0) {
                continue;
            }
            if (*(int *)((char *)object + 0x324) != track_count) {
                continue;
            }
            for (node = 1; node < track_count; node++) {
                *(int *)((char *)object + 0x328 + node * 4) = 0;
            }
            *(int *)((char *)object + 0x324) = 1;
            (*(int *)((char *)object + 0x40c))++;
        } else {
            *(int *)((char *)object + 0x328 + index * 4) = 1;
            (*(int *)((char *)object + 0x324))++;
        }
    }
    if (found == 0) {
        *(int *)((char *)object + 0x3f0) = -1;
    }
    {
        int index;
        int next;
        int dx;
        int dy;
        int segment;
        int distance;

        index = *(int *)((char *)object + 0x320);
        next = index + 1;
        if (next >= track_count) {
            next -= track_count;
        }
        dx = track_nodes[next].x - track_nodes[index].x;
        dy = track_nodes[next].y - track_nodes[index].y;
        segment = (int)sqrt((double)dx * track_scale * ((double)dx * track_scale) +
                            (double)dy * track_scale * ((double)dy * track_scale));
        dx = *(int *)((char *)object + 0x498) - track_nodes[index].x;
        dy = *(int *)((char *)object + 0x4a0) - track_nodes[index].y;
        distance = (int)sqrt((double)dx * track_scale * ((double)dx * track_scale) +
                             (double)dy * track_scale * ((double)dy * track_scale));
        *(float *)((char *)object + 0x404) =
            (float)distance / (float)segment;
        *(float *)((char *)object + 0x408) =
            (float)(100 * *(int *)((char *)object + 0x40c)) +
            (float)index + *(float *)((char *)object + 0x404);
    }
}
