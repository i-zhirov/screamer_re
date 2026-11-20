/* The rebuilt `track_timer_update` module.
 *
 * The original sits at linear 0x1960c in STARTH.EXE and holds 32 instructions
 * and 105 bytes up to the `ret` at 0x19674.  Three pad bytes follow the `ret`,
 * and the next record starts at 0x19678, so the store record keeps the
 * 108-byte extent and sets no separate function size.  The identity check uses
 * 32 instructions.
 *
 * The only direct caller is `player_lap_progress_update` at 0x194a0, with the
 * call sites at 0x1955c and 0x19592.  That function runs in the vehicle state
 * chain, so the record takes the section `The vehicle state chain`.
 *
 * The function returns at once when the race position `race_position` plus one
 * equals the lap index at `+0x684`.  Otherwise it divides the timer at
 * `+0x790` by the divisor 5.0 at object-3 offset `0x8b4`, adds the integer step,
 * and clears the effect timer index 0x16 through 0x24d7c.
 *
 * The mode byte `DAT_0011d260` selects the step.  When bits 0x60 are set, the
 * step comes from object-3 offset 0xcf50.  Otherwise the step comes from
 * object-3 offset 0xcf4c.  The minimal harness image does not load the constant
 * segment, so the module reads the divisor and the two steps from the globals
 * `track_timer_divisor`, `track_timer_step_mode`, and
 * `track_timer_step_default`.
 *
 * `player_lap_progress_update` already names the callee `track_timer_update`.
 * The timer field, the two step globals, and the effect timer clear support the
 * name.  The original has no symbol, so the record keeps the name with the
 * confidence `inferred`.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses the x87 stack for the divide and for the integer-to-float add,
 * so the module links the shared `fpu_stub.asm` for the run-time symbols
 * `__8087`, `__init_387_emulator`, and `_fltused_`.  The module uses no
 * assembly implementation.
 */
extern int race_position;
extern unsigned char DAT_0011d260;
extern float track_timer_divisor;
extern int track_timer_step_mode;
extern int track_timer_step_default;
extern void effect_timer_clear(int index);

void track_timer_update(void *object)
{
    int step;

    if (race_position + 1 == *(int *)((char *)object + 0x684)) {
        return;
    }
    *(float *)((char *)object + 0x790) =
        *(float *)((char *)object + 0x790) / track_timer_divisor;
    if ((DAT_0011d260 & 0x60) != 0) {
        step = track_timer_step_mode;
    } else {
        step = track_timer_step_default;
    }
    *(float *)((char *)object + 0x790) =
        *(float *)((char *)object + 0x790) + (float)step;
    effect_timer_clear(0x16);
}
