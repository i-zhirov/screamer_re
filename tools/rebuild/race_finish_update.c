/* The rebuilt `race_finish_update` module.
 *
 * The original sits at linear 0x21058 in STARTH.EXE and holds 114 instructions
 * and 391 bytes up to the `ret` at 0x211de.  The next function starts at
 * 0x211e0, so the store record keeps the 392-byte extent and sets no function
 * size.  The identity check uses 114 instructions.
 *
 * The only direct caller at 0x20033 is the frame update at 0x1ff70, which runs
 * this function last in the race frame sequence, after the two transition
 * handlers at 0x20ec8 and 0x20f98.  No named function sits between the frame
 * update and the target, so the record takes the section Program functions.
 *
 * The gate is bit 2 of `DAT_00118e1c`.  The two transition handlers set that
 * bit: the handler at 0x20ec8 runs after the final lap sets bit 3, and the
 * handler at 0x20f98 runs after the finish sets bit 4.  The function therefore
 * runs the finish delay.
 *
 * While the delay runs, the function draws one mode-specific message.  When bit
 * 4 of `DAT_00118e1d` is set it draws the text pointer `DAT_00118fec` through
 * `panel_draw_centered`.  Otherwise it switches on the mode `DAT_0011d260` and
 * calls the overlay for mode 1, 2, 4, 0x10, 0x20, or 0x40.  The overlays format
 * the car table at `DAT_00118f94` and the clock text, so they draw the race
 * standings.
 *
 * The function then adds the frame delta `DAT_00118e08` to the state timer
 * `DAT_00118e28`.  A timer below the 5.0 constant at object-3 offset 0xd01 ends
 * the function.  A non-empty effect queue `DAT_00119080` also ends the function,
 * so the delay waits for the sound effects.
 *
 * When the timer passes 5.0 and the queue is empty, the function sets the slot
 * `DAT_0010ffa4` to 1 and, in the traffic mode, runs the traffic step at
 * 0x1eae0 with the count 3.  It then walks the 0x2a8-byte traffic records at
 * `DAT_00113cd8` up to the count `DAT_00115218`.  A record that is not finished
 * either takes the value -3 when the flag at `+0x16` bit 1 is set, or clears
 * the finished flag.  A set finished flag sets bits 1 and 2 of
 * `DAT_00118e1d`, which makes the frame update skip the race update.
 *
 * The function finally releases the race resource `DAT_001100ec` through
 * 0x1f880 when the handle is not -1.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The module
 * uses the shared FPU stub, because it needs no truncation and no math helper.
 */
extern unsigned char DAT_00118e1c;
extern unsigned char DAT_00118e1d;
extern float DAT_00118e08;
extern float DAT_00118e28;
extern float race_finish_limit;
extern int DAT_0011d260;
extern int DAT_0010ffa4;
extern int DAT_00118fec;
extern int DAT_00115218;
extern int DAT_001100ec;
extern char DAT_00113cd8[];

extern void panel_draw_centered(int text, int y, int font, int color);
extern void race_draw_1(void);
extern void race_draw_2(void);
extern void race_draw_4(void);
extern void race_draw_16(void);
extern void race_draw_32(void);
extern void race_draw_64(void);
extern int effect_queue_head(void);
extern void traffic_step(int count, char *base);
extern int resource_release(int handle);

void race_finish_update(void)
{
    int mode;
    int finished;
    int *record;
    int *end;

    if ((DAT_00118e1c & 4) == 0) {
        return;
    }

    if ((DAT_00118e1d & 0x10) != 0) {
        panel_draw_centered(DAT_00118fec, 0x5a, 2, 0);
    } else {
        mode = DAT_0011d260;
        switch (mode) {
        case 1:
            race_draw_1();
            break;
        case 2:
            race_draw_2();
            break;
        case 4:
            race_draw_4();
            break;
        case 0x10:
            race_draw_16();
            break;
        case 0x20:
            race_draw_32();
            break;
        case 0x40:
            race_draw_64();
            break;
        }
    }

    DAT_00118e28 = DAT_00118e08 + DAT_00118e28;
    if (DAT_00118e28 < race_finish_limit) {
        goto release;
    }
    if (effect_queue_head() != -1) {
        goto release;
    }

    finished = 1;
    if ((DAT_0011d260 & 0x10) != 0 && (DAT_00118e1d & 0x10) == 0) {
        traffic_step(3, DAT_00113cd8);
        DAT_0010ffa4 = 1;
        record = (int *)DAT_00113cd8;
        end = (int *)(DAT_00113cd8 + DAT_00115218 * 0x2a8);
        while (record < end) {
            if (*record >= 0) {
                if ((*(unsigned char *)((char *)record + 0x16) & 2) != 0) {
                    *record = -3;
                } else {
                    finished = 0;
                }
            }
            record = (int *)((char *)record + 0x2a8);
        }
    }
    if (finished != 0) {
        DAT_00118e1d = DAT_00118e1d | 6;
    }

release:
    if (DAT_001100ec != -1) {
        DAT_001100ec = resource_release(DAT_001100ec);
    }
}
