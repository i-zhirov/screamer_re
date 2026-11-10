/* The rebuilt `cd_audio_table_build` module.
 *
 * The original sits at linear 0x262fc in STARTH.EXE.  The first body holds 34
 * instructions and 110 bytes up to the `ret` at 0x26369.  A conditional branch
 * at 0x2635c enters a second block at 0x2636a, and that block ends with the
 * `ret` at 0x263c8.  The next recorded function starts at 0x263cc, so the store
 * record keeps the 208-byte extent and sets no separate function size.  The
 * identity check uses 34 instructions.
 *
 * The only direct caller is `cd_check_screen` at 0x15874, which calls this
 * function at 0x15977 after the CD audio stop.  The caller sits in the section
 * race_loop callees, so this module takes the same section.
 *
 * The function builds the MSCDEX requests at `DAT_001666a4`.  It first tests
 * bit 0 of `DAT_00166688`.  A set bit returns 1 and sets the bit again.  It then
 * tries up to three times: the audio disc info request through 0x260a0 and the
 * audio track info request through 0x26120.  A set bit 15 of the status word
 * repeats the pair, and three failures return 1 and set the flag.
 *
 * On success the first track at +0x41 and the last track at +0x42 of the
 * response come from the block.  The last track is clamped to 0x14 and stored
 * at `DAT_00166692`, and the first track is stored at `DAT_0016668e`.
 *
 * The scan calls the track info request through 0x261a8 for each track.  The
 * control byte at +0x46 bit 6 marks a data track.  When every track is a data
 * track, the function returns 1 and sets the flag.  The scan stops at the first
 * audio track, and the second block builds the 8-byte table at
 * `DAT_001666c0` from that track to the last track: the start time of the track
 * and the duration to the next start.  The last duration runs to the lead-out
 * time from the second audio track info request.  The start time and the
 * duration come from the minute, second, and frame fields through the helper at
 * 0x2626c.  The second block returns 0.
 *
 * The original has no EBP frame and saves the registers with `pushal`, so the
 * module builds without `-of+`.  The function holds no floating-point
 * instruction, so it needs no FPU stub and no assembly.
 */
extern int DAT_00166688;
extern int DAT_0016668e;
extern int DAT_00166692;
extern unsigned int DAT_001666a4;
extern int DAT_001666c0[];

extern void sub_260a0(void);
extern int sub_26120(void);
extern void sub_261a8(int track);
extern int sub_2626c(int minute, int second, int frame);

int cd_audio_table_build(void)
{
    unsigned char *fields;
    int *table;
    int track;
    int last;
    int slot;
    int start;

    if (DAT_00166688 & 1) {
        DAT_00166688 = 1;
        return 1;
    }
    track = 3;
    while (1) {
        sub_260a0();
        if ((sub_26120() & 0x8000) == 0) {
            break;
        }
        track = track - 1;
        if (track == 0) {
            DAT_00166688 = 1;
            return 1;
        }
    }
    fields = (unsigned char *)DAT_001666a4 + 0x40;
    track = fields[1];
    last = fields[2];
    if (last >= 0x14) {
        last = 0x14;
    }
    DAT_00166692 = last;
    DAT_0016668e = track;
    while (1) {
        sub_261a8(track);
        if ((fields[6] & 0x40) == 0) {
            break;
        }
        track = track + 1;
        if (track > last) {
            DAT_00166688 = 1;
            return 1;
        }
    }
    DAT_0016668e = track;
    table = DAT_001666c0;
    slot = 0;
    start = sub_2626c(fields[4], fields[3], fields[2]);
    table[slot] = start;
    track = track + 1;
    while (track <= last) {
        sub_261a8(track);
        start = sub_2626c(fields[4], fields[3], fields[2]);
        table[slot + 1] = start - table[slot];
        slot = slot + 2;
        start = sub_2626c(fields[4], fields[3], fields[2]);
        table[slot] = start;
        track = track + 1;
    }
    sub_26120();
    start = sub_2626c(fields[5], fields[4], fields[3]);
    table[slot + 1] = start - table[slot];
    return 0;
}
