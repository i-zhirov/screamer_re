/* The rebuilt `race_intro_update` module.
 *
 * The original sits at linear 0x20d20 in STARTH.EXE and holds 106 instructions
 * and 421 bytes up to the `ret` at 0x20ec4.  The next function starts at
 * 0x20ec8, so the store record keeps the 424-byte extent and sets no function
 * size.  The identity check uses 106 instructions.
 *
 * The only direct caller at 0x2001f is the frame update at 0x1ff70, which runs
 * this function after the race HUD draw at 0x23420.  No named function sits
 * between the frame update and the target, so the record takes the section
 * Program functions.
 *
 * The function runs only while the byte `DAT_00118e1c` holds bit 1.  The race
 * start `race_session_start` at 0x128e0 sets the word `DAT_00118e1c` to 0x102
 * and clears the timer `DAT_00118e24`, so the function runs for the first
 * seconds of the race.  Each call adds the frame delta `DAT_00118e08` to the
 * timer and stores the half-second bucket at `DAT_00118e20`.
 *
 * The bucket 3 sets bit 0 of `DAT_00118e1c` and clears the field `+0x4f4` of
 * every 0x590-byte car record at `DAT_00110ab8`.  The bucket 4 toggles bit 1
 * off, sets the bucket to 3, and ends the sequence.  A changed bucket pushes
 * the event `0x18e20` through 0x247ac.
 *
 * The function then sets the world view rectangle at `DAT_0000056c` through
 * `DAT_00000578` to the full 640 by 400 screen, puts the centered sprite at
 * (0xf0, 0x6c) with the bucket as its mode in `DAT_0011cb18` through
 * `DAT_0011cb20`, and flushes the sprite list through 0x25af0.
 *
 * While bit 0 of `DAT_00118e1c` is clear, the function fills the field `+0x4f4`
 * of every car record with `sin(index * 0.5 * timer) * 15000.0`.  The two
 * constants sit at object-3 offsets 0xcf1 and 0xcf9 and hold 0.5 and 15000.0.
 *
 * The original calls the truncate helper at 0x4d9f4 before each `fistp` and the
 * sine helper at 0x4da72.  This harness links no math library, so the local
 * `race_intro_update_fpu.asm` makes `__CHP` truncate toward zero and supplies
 * `sin` from the hardware.  The constants arrive through globals, because the
 * minimal image does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern unsigned char DAT_00118e1c;
extern int DAT_00118e20;
extern float DAT_00118e24;
extern float DAT_00118e08;
extern int DAT_0010f8ec;
extern int DAT_0010634c;
extern unsigned char DAT_00110ab8[];
extern int DAT_0000056c;
extern int DAT_00000570;
extern int DAT_00000574;
extern int DAT_00000578;
extern int DAT_0011cb18;
extern int DAT_0011cb1c;
extern int DAT_0011cb20;

extern double race_intro_phase;
extern double race_intro_amplitude;

extern void effect_push(int value, int subtype);
extern void panel_update(void);
extern double sin(double value);

void race_intro_update(void)
{
    char *record;
    int previous;
    int bucket;

    if ((DAT_00118e1c & 2) == 0) {
        return;
    }

    previous = ((int)DAT_00118e24) / 2;
    DAT_00118e24 = DAT_00118e08 + DAT_00118e24;
    bucket = ((int)DAT_00118e24) / 2;
    DAT_00118e20 = bucket;

    switch (bucket) {
    case 3:
        DAT_00118e1c = DAT_00118e1c | 1;
        DAT_0010634c = 0;
        while (DAT_0010634c < DAT_0010f8ec) {
            record = (char *)DAT_00110ab8 + DAT_0010634c * 0x590;
            *(int *)(record + 0x4f4) = 0;
            DAT_0010634c++;
        }
        break;
    case 4:
        DAT_00118e1c = DAT_00118e1c ^ 2;
        DAT_00118e20 = 3;
        break;
    }

    if (previous != DAT_00118e20 || DAT_00118e24 == DAT_00118e08) {
        effect_push(DAT_00118e20, 0);
    }

    DAT_0000056c = 0;
    DAT_00000570 = 0x280;
    DAT_00000574 = 0;
    DAT_00000578 = 0x190;
    DAT_0011cb18 = 0xf0;
    DAT_0011cb1c = 0x6c;
    DAT_0011cb20 = DAT_00118e20;
    panel_update();

    if ((DAT_00118e1c & 1) != 0) {
        return;
    }

    DAT_0010634c = 0;
    while (DAT_0010634c < DAT_0010f8ec) {
        record = (char *)DAT_00110ab8 + DAT_0010634c * 0x590;
        *(float *)(record + 0x4f4) = (float)(
            sin((double)DAT_0010634c * race_intro_phase
                * (double)DAT_00118e24) * race_intro_amplitude);
        DAT_0010634c++;
    }
}
