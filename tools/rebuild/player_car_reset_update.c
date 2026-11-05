/* The rebuilt `player_car_reset_update` module.
 *
 * The original sits at linear 0x18528 in STARTH.EXE.  The record keeps the
 * 912-byte extent that covers the three return blocks, and the identity check
 * uses the first 168 instructions up to the ret at 0x1882f.  The function
 * branches on the state at `+0x148` through the jump table at 0x8510:
 *
 *     state 1  the update block at 0x18670
 *     state 2  the init block at 0x18550, then the update block
 *     state 3  the default return at 0x188b0
 *     state 4  the default return at 0x188b0
 *     state 5  the smoke block at 0x18830
 *     state 6  the clear block at 0x18896
 *
 * The only direct caller is the effect group at 0x15bac.  The player physics
 * group at 0x15a80 sets the state to 2 at 0x16da4 when the heading error times
 * the speed passes the speed threshold at `+0x180`, so the init block is the
 * start of the reset.
 *
 * The init block looks the sound entry 8 up through `table_lookup`, enqueues
 * the effect 9 with the value 3 and 0.5 through 0x247fc, sets the state to 1,
 * copies the slot index `DAT_00110aa4` to `+0x160`, sets `DAT_00001e18` when
 * the replay flag `DAT_00118e6c` is zero, and scales the speed `+0x50c` by
 * 0.6.  It publishes the position `+0x4b0` and `+0x4b8` through the globals
 * `DAT_00108038` and `DAT_00108040`, calls 0x120c0 with 0, and copies the
 * direction `DAT_00108048` to the heading `+0x4cc`.  It sets the timer `+0x14c`
 * to 3000.0, clears the counter `+0x150`, sets the two velocities `+0x154` and
 * `+0x15c` to 12.0, clears the offsets `+0x124`, `+0x128`, and `+0x12c`, and
 * clears the accumulator `+0x134` and the effect field `+0x170`.
 *
 * The update block adds the scaled timer to the accumulator, subtracts 6000.0
 * times the frame delta from the timer, damps the two velocities with the
 * coefficient at `DAT_0010cf08 + 0x184`, and integrates them into `+0x124` and
 * `+0x12c`.  When the accumulator stops above zero, it returns.  Otherwise it
 * starts the four effect slots through 0x19828, increments the counter, reflects
 * the timer by 0.4, clears the offsets `+0x124` and `+0x12c`, selects -0.6 on
 * an odd counter or 0.6 on an even counter, and sets the velocity `+0x154` to
 * -1.0.  When the counter passes 3, it clears the state and, with the flag
 * `DAT_00118e1c` bit 0 set and the replay flag clear, marks the slot record at
 * `DAT_00101db4 + 5 * +0x160`.
 *
 * State 5 runs only with a non-zero gear `+0x7ac`.  It allocates the effect
 * handle `+0x7d0` through `table_lookup` when the handle is -1, toggles the low
 * five bits of `+0x4bc`, advances the heading by 0.3 times `+0x108`, and starts
 * the four effect slots.  State 6 clears `+0x4bc` and the state.
 *
 * The original uses the jump table at 0x8510.  The rebuild uses an if-else
 * chain, because the same V2 jump table fault shows in the earlier switch
 * modules.
 *
 * The single conversion to an integer uses the Watcom helper at 0x4d9f4,
 * which truncates toward zero.  The module uses a local truncating FPU stub,
 * because the shared stub rounds to nearest.
 *
 * The constants arrive through globals, because the minimal image does not
 * load the constant segment.  The original has the EBP frame, so the module
 * builds with `-of+`.
 */
extern int table_lookup(int index);
extern void sound_enqueue(int id, int value, float extra);
extern void track_direction(int value);
extern void effect_start(void *object);

extern float DAT_00118e04;
extern float *DAT_0010cf08;
extern unsigned char DAT_00118e1c;
extern int DAT_00118e6c;
extern int DAT_00110aa4;
extern int DAT_00001e18;
extern int DAT_00108038;
extern int DAT_00108040;
extern float DAT_00108048;
extern int DAT_00101db4[];

extern double reset_speed_scale;
extern float reset_decrement;
extern double reset_bounce;
extern double reset_heading_scale;

void player_car_reset_update(void *object)
{
    char *base = (char *)object;
    int state;

    state = *(int *)(base + 0x148);
    if (state == 2) {
        table_lookup(8);
        sound_enqueue(9, 3, 0.5f);
        *(int *)(base + 0x148) = 1;
        *(int *)(base + 0x160) = DAT_00110aa4;
        if (DAT_00118e6c == 0) {
            DAT_00001e18 = 1;
        }
        *(float *)(base + 0x50c) = (float)(
            (double)*(float *)(base + 0x50c) * reset_speed_scale);
        DAT_00108038 = *(int *)(base + 0x4b0);
        DAT_00108040 = *(int *)(base + 0x4b8);
        track_direction(0);
        *(float *)(base + 0x4cc) = DAT_00108048;
        *(float *)(base + 0x14c) = 3000.0f;
        *(int *)(base + 0x150) = 0;
        *(float *)(base + 0x15c) = 12.0f;
        *(float *)(base + 0x154) = *(float *)(base + 0x15c);
        *(float *)(base + 0x128) = 0.0f;
        *(float *)(base + 0x12c) = *(float *)(base + 0x128);
        *(float *)(base + 0x124) = *(float *)(base + 0x12c);
        *(int *)(base + 0x134) = 0;
        *(int *)(base + 0x170) = 0;
        state = 1;
    }
    if (state == 1) {
        *(int *)(base + 0x134) = (int)(*(float *)(base + 0x14c)
            * DAT_00118e04 + *(int *)(base + 0x134));
        *(float *)(base + 0x14c) = *(float *)(base + 0x14c)
            - DAT_00118e04 * reset_decrement;
        *(float *)(base + 0x154) = *(float *)(base + 0x154)
            * *(float *)((char *)DAT_0010cf08 + 0x184);
        *(float *)(base + 0x124) = *(float *)(base + 0x124)
            + *(float *)(base + 0x154) * DAT_00118e04;
        *(float *)(base + 0x15c) = *(float *)(base + 0x15c)
            * *(float *)((char *)DAT_0010cf08 + 0x184);
        *(float *)(base + 0x12c) = *(float *)(base + 0x12c)
            + *(float *)(base + 0x15c) * DAT_00118e04;
        if (*(int *)(base + 0x134) <= 0) {
            effect_start(object);
            *(int *)(base + 0x150) = *(int *)(base + 0x150) + 1;
            *(int *)(base + 0x134) = 0;
            *(float *)(base + 0x14c) = -(*(float *)(base + 0x14c)
                * reset_bounce);
            *(float *)(base + 0x12c) = 0.0f;
            *(float *)(base + 0x124) = *(float *)(base + 0x12c);
            if (*(int *)(base + 0x150) & 1) {
                *(float *)(base + 0x15c) = (float)(-0.6);
            } else {
                *(float *)(base + 0x15c) = (float)0.6;
            }
            *(float *)(base + 0x154) = -1.0f;
        }
        if (*(int *)(base + 0x150) > 3) {
            *(int *)(base + 0x148) = 0;
            if ((DAT_00118e1c & 1) != 0 && DAT_00118e6c == 0) {
                DAT_00101db4[5 * *(int *)(base + 0x160)] = 1;
            }
        }
    }
    if (state == 5) {
        if (*(int *)(base + 0x7ac) != 0) {
            if (*(int *)(base + 0x7d0) == -1) {
                *(int *)(base + 0x7d0) = table_lookup(0);
            }
            *(unsigned char *)(base + 0x4bc) ^= 0x1f;
            *(float *)(base + 0x4cc) = (float)(
                (double)*(float *)(base + 0x4cc)
                + (double)*(float *)(base + 0x108)
                    * reset_heading_scale);
            effect_start(object);
        }
    }
    if (state == 6) {
        *(int *)(base + 0x4bc) = 0;
        *(int *)(base + 0x148) = 0;
    }
}
