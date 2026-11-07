/* The rebuilt `player_lap_progress_update` module.
 *
 * The original sits at linear 0x194a0 in STARTH.EXE and holds 103 instructions
 * and 363 bytes up to the `ret` at 0x1960a.  The next function starts at
 * 0x1960c, so the store record keeps the 364-byte extent and sets no separate
 * function size.  The identity check uses 103 instructions.
 *
 * The only known caller is `player_track_node_update` at 0x1821c.  That
 * function selects a track node and stores the status at +0x780, the target
 * index at +0x668, the node flags at +0x678, and the segment ratio at +0x67c.
 * This function consumes those fields in the vehicle state chain.
 *
 * A status of 1 marks the target node in the second visited chain at +0x5a0
 * and increments the lap counter at +0x59c.  When the node flags hold bit 2,
 * the first call sets the latch at +0x784 and the later calls push the effect 4
 * with the subtype 2 below the timer 3.0, the subtype 1 above the timer 15.0,
 * and the subtype 0 between the two limits.  Each such call updates the timer
 * through 0x1960c.
 *
 * A status of 2 at the target node 0 runs only when the lap counter reaches the
 * node count at 0x7ed0.  It updates the timer through 0x1960c and starts the
 * new lap through `race_lap_complete` at 0x19678.
 *
 * The function then forms the race progress at +0x680 from the lap index
 * +0x684, the current node index +0x598, and the segment ratio +0x67c when the
 * current node is marked in the second chain.  It clears the status at +0x780.
 *
 * The two timer limits arrive through the globals `lap_timer_low` and
 * `lap_timer_high`, because the minimal harness image does not load the
 * constant segment.  The original reads the limits at object-3 offsets 0x8ac
 * and 0x8b0, which hold 3.0 and 15.0.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int lap_node_count;
extern float lap_timer_low;
extern float lap_timer_high;
extern void effect_push(int type, int subtype, float value);
extern void track_timer_update(void *object);
extern void race_lap_complete(void *object);

void player_lap_progress_update(void *object)
{
    int state;
    int subtype;
    int target;
    float timer;
    int node;
    float progress;

    state = *(int *)((char *)object + 0x780);
    if (state == 1) {
        target = *(int *)((char *)object + 0x668);
        *(int *)((char *)object + 0x5a0 + target * 4) = 1;
        *(int *)((char *)object + 0x59c) =
            *(int *)((char *)object + 0x59c) + 1;
        if ((*(unsigned char *)((char *)object + 0x678) & 2) != 0) {
            if (*(int *)((char *)object + 0x784) == 0) {
                *(int *)((char *)object + 0x784) = 1;
            } else {
                timer = *(float *)((char *)object + 0x790);
                if (timer < lap_timer_low) {
                    subtype = 2;
                } else if (timer > lap_timer_high) {
                    subtype = 1;
                } else {
                    subtype = 0;
                }
                effect_push(4, subtype, 0.0f);
            }
            track_timer_update(object);
        }
    } else if (state == 2) {
        if (*(int *)((char *)object + 0x668) == 0
            && lap_node_count == *(int *)((char *)object + 0x59c)) {
            track_timer_update(object);
            race_lap_complete(object);
        }
    }
    node = *(int *)((char *)object + 0x598);
    if (*(int *)((char *)object + 0x5a0 + node * 4) != 0) {
        progress = (float)(100 * *(int *)((char *)object + 0x684))
                   + (float)node
                   + *(float *)((char *)object + 0x67c);
        *(float *)((char *)object + 0x680) = progress;
    }
    *(int *)((char *)object + 0x780) = 0;
}
