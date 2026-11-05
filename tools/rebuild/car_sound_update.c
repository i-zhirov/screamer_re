/* The rebuilt `car_sound_update` module.
 *
 * The original sits at linear 0x242bc in STARTH.EXE and holds 215 instructions
 * and 829 bytes up to the `ret` at 0x245f8.  The record keeps the 836-byte
 * extent and sets no function size.
 *
 * The only caller at 0x15bf1 is the effect group at 0x15bac.  The group runs
 * the player car steps on the player object and calls this function last, when
 * the byte `DAT_00118e1c` bit 0 is set.  The five view groups call that group
 * on the player object at object-3 offset 0xf91c, so the module takes the
 * section The vehicle state chain.
 *
 * The function updates the sound-effect timers of the player car.  It reads the
 * angle `+0x13c` and the heading `+0x4cc` through the helper at 0x22664, the
 * speed `+0x50c`, the engine speed `+0x688`, the state `+0x678`, the pitch term
 * `+0x4f4`, the progress `+0x684`, the base height `+0x680`, and the value
 * `+0x790`.  The byte `DAT_0011d260` selects the race-mode branches, and the
 * latch `DAT_00119060` holds the two-step gear trigger.
 *
 * The timers are the functions at 0x24c4c, 0x24cc8, and 0x24d30.  They enqueue
 * an effect through 0x247fc, which filters the queue at 0x19080 and resolves
 * the table entry at 0x1f718.  The module records the callees as the sound
 * effect timers.
 *
 * The module scans the 0x590-byte car records at `DAT_00110ab8` up to
 * `DAT_00110ab8 + DAT_0010f8ec * 0x590` for a record near the player progress
 * `+0x684`, and it reports the result through the timer 0x17.
 *
 * The FPU helper at 0x4d9f4 truncates the pitch term, so the module uses a
 * local truncating `__CHP` stub.  The float constants live in globals, because
 * the harness does not load the constant segment.  The original has the EBP
 * frame, so the module builds with `-of+`.
 */
extern int car_audio_gate;
extern unsigned char car_audio_mode;
extern int car_audio_latch;
extern int car_record_count;
extern unsigned char car_records[];
extern int car_compare_a;
extern int car_compare_b;
extern int car_compare_c;
extern int car_flag_a;
extern float car_angle_bias;
extern float car_speed_low;
extern float car_angle_limit;
extern double car_rate_scale;
extern double car_limit_scale;
extern float car_rate_base;
extern float car_timer_value;

extern int angle_within(float a, float b, float tolerance);
extern int effect_countdown(int enabled, int id, float limit);
extern int effect_after(int enabled, int id, float limit);
extern int effect_trigger(int id, int value);

void car_sound_update(void *object)
{
    char *record;
    char *end;
    int near_target;
    int limit;
    int found;

    if (car_audio_gate == 0)
        return;

    near_target = angle_within(
        *(float *)((char *)object + 0x13c) + car_angle_bias,
        *(float *)((char *)object + 0x4cc),
        0.785398163f);

    effect_after(near_target, 0x10, 5.0f);
    effect_after(
        *(float *)((char *)object + 0x50c) < car_speed_low, 0x11, 8.0f);

    if ((car_audio_mode & 0x17) != 0) {
        if (*(float *)((char *)object + 0x790) <= car_angle_limit)
            effect_trigger(0x16, 3);
    }

    if ((car_audio_mode & 0x13) != 0) {
        if (effect_countdown(
                *(int *)((char *)object + 0x688) == 1, 0x15, 5.0f) != 0)
            car_audio_latch = 1;
    }

    if (effect_countdown(
            car_audio_latch != 0
                && *(int *)((char *)object + 0x688) > 1,
            0x18, 5.0f) != 0)
        car_audio_latch = 0;

    if ((car_audio_mode & 0x20) != 0) {
        if (car_compare_a == car_compare_b)
            effect_trigger(0x1a, 3);
    } else if ((car_audio_mode & 0x40) != 0) {
        if (car_compare_c == *(int *)((char *)object + 0x7c0)
            && car_compare_b == 0)
            effect_trigger(0x1a, 3);
    }

    if (near_target == 0)
        effect_countdown(
            *(int *)((char *)object + 0x678) & 4, 0x14, 0.0f);

    effect_countdown(
        (int)*(float *)((char *)object + 0x4f4), 0x0a,
        (float)(car_rate_base * car_rate_scale));

    limit = *(int *)((char *)object + 0x788);
    if (limit <= 1)
        limit = 2;
    effect_countdown(
        *(int *)((char *)object + 0x688) > limit, 0x0d, 8.0f);

    if ((car_audio_mode & 3) != 0) {
        found = 0;
        record = (char *)car_records;
        end = record + car_record_count * 0x590;
        while (record < end) {
            if (*(int *)((char *)object + 0x684)
                > *(int *)(record + 0x40c)) {
                float player_value =
                    (float)(*(int *)((char *)object + 0x684) * 100);
                float record_value =
                    (float)(*(int *)(record + 0x40c) * 100);
                if (*(float *)(record + 0x408) - record_value
                    < *(float *)((char *)object + 0x680)
                        - player_value) {
                    found = 1;
                    break;
                }
            }
            if (*(int *)((char *)object + 0x684)
                    - *(int *)(record + 0x40c) > 1) {
                found = 1;
                break;
            }
            record += 0x590;
        }
        if (found == 0)
            record = (char *)car_records;
        effect_countdown(found, 0x17, 2.0f);
    }

    effect_after(
        (double)car_timer_value > car_limit_scale, 0x0f, 0.0f);

    if ((car_audio_mode & 0x20) != 0 && car_flag_a != 0)
        effect_trigger(0x13, 3);
}
