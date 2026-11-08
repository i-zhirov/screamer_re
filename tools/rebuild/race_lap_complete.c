/* The rebuilt `race_lap_complete` module.
 *
 * The original sits at linear 0x19678 in STARTH.EXE and holds 124 instructions
 * and 431 bytes up to the `ret` at 0x19826.  The next function starts at
 * 0x19828, so the store record keeps the 432-byte extent and sets no function
 * size.  The identity check uses 124 instructions.
 *
 * The only direct caller at 0x1959a is the lap consumer at 0x194a0.  That
 * function runs in the vehicle state chain after `player_track_node_update`,
 * and it calls this function when the lap counter reaches the node count.
 *
 * The function starts a new lap.  It sets the lap counter at +0x59c to 1,
 * clears the visited flags at +0x5a0 from index 1 up to the node count
 * `race_lap_count`, and increments the lap index at +0x684.
 *
 * The current lap time at +0x794 updates the car best lap at +0x798 when it is
 * smaller.  The lap is also compared with the global best lap `race_best_lap`.
 * A smaller lap replaces the global value, formats the car best lap as
 * `%02d:%02d:%02d` into `race_lap_text`, and pushes the effect 7 with the
 * subtype 3 and the value 0.2 through 0x247fc.  The function then clears the
 * current lap time.
 *
 * The lap index is compared with the race position `race_lap_position`.  An
 * equal index pushes the effect 5 with the subtype 2, the next index sets bit
 * 8 of `race_lap_flags`, and any other index pushes the effect 5 with the
 * subtype that tells whether the best lap was recorded.  The format call is
 * `sprintf` at 0x4d32c.
 *
 * The name follows from the lap counter, the best lap, the time format, and
 * the caller.  The original holds no symbol for this function, so the record
 * keeps the name with the confidence inferred.
 *
 * The original truncates each float through the helper at 0x4d9f4.  The shared
 * `fpu_stub.asm` leaves round-to-nearest, so this harness uses the local
 * truncating `__CHP` stub.  The 100.0 scale arrives through a global, because
 * the minimal image does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int race_lap_count;
extern float race_best_lap;
extern int race_lap_position;
extern unsigned char race_lap_flags;
extern char race_lap_text[];
extern char race_time_format[];
extern float race_centi_scale;
extern int sprintf(char *buffer, const char *format, ...);
extern void effect_push(int type, int subtype, float value);

void race_lap_complete(void *object)
{
    float current;
    float best;
    int index;
    int recorded;
    int whole;
    int minutes;
    int seconds;
    int centis;

    recorded = 0;
    *(int *)((char *)object + 0x59c) = 1;
    for (index = 1; index < race_lap_count; index++) {
        *(int *)((char *)object + 0x5a0 + index * 4) = 0;
    }
    *(int *)((char *)object + 0x684) = *(int *)((char *)object + 0x684) + 1;

    current = *(float *)((char *)object + 0x794);
    best = *(float *)((char *)object + 0x798);
    if (current < best) {
        best = current;
        *(float *)((char *)object + 0x798) = best;
    }
    if (current < race_best_lap) {
        race_best_lap = current;
        whole = (int)best;
        minutes = whole / 60;
        seconds = whole % 60;
        centis = (int)((best - (float)whole) * race_centi_scale);
        sprintf(race_lap_text, race_time_format, minutes, seconds, centis);
        effect_push(7, 3, 0.2f);
        recorded = 1;
    }
    *(float *)((char *)object + 0x794) = 0.0f;

    index = *(int *)((char *)object + 0x684);
    if (race_lap_position == index) {
        effect_push(5, 2, 0.0f);
    } else if (race_lap_position + 1 == index) {
        race_lap_flags = race_lap_flags | 8;
    } else {
        effect_push(5, recorded != 0, 0.0f);
    }
}
