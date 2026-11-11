/* The rebuilt `cd_audio_tick` module.
 *
 * The original sits at linear 0x25f90 in STARTH.EXE.  The first body holds 18
 * instructions and 60 bytes up to the `ret` at 0x25fcb.  The identity window
 * uses the plan count 18.  The function continues in the blocks at 0x25fcc,
 * 0x25ff2, and 0x2600b, and the whole body ends with the `ret` at 0x2600c.
 * The store record keeps the 132-byte extent and sets no separate function
 * size.
 *
 * The direct callers are `cd_check_screen` at 0x15874 with the call sites
 * 0x1593e, 0x1594e, and 0x15970, `options_screen` at 0x21338 with the call
 * sites 0x217ae, 0x21845, 0x218a0, and 0x218b5, and the request block at
 * 0x26068 with the call site 0x26097.  The callers sit in the section
 * race_loop callees, so the module takes the same section.
 *
 * The function increments the tick counter at `DAT_0016000d`.  While the
 * counter is below 4, it reads the audio Q-channel status through 0x261e4 and
 * sets the state flag at `DAT_00166688` when the status is 2 or 0x0f.
 *
 * On the fourth tick, the function clears the counter and reads the audio disc
 * info through 0x260a0.  A low byte of 0xff marks a missing disc: the function
 * sets the state flag and calls 0x262a8 with 0 when the gate at
 * `DAT_0016669a` is set.  With a disc, the function returns when the state flag
 * bit 0 is clear.  Otherwise it reads the Q-channel status; a non-zero status
 * returns, and a zero status clears the state flag, builds the track table
 * through 0x262fc, and calls 0x262a8 with 0 when the gate is set.
 *
 * The tick counter, the two state cells, and the four callees support the name
 * `cd_audio_tick`.  The original has no symbol, so the record keeps the name
 * with the confidence inferred.
 *
 * The original has no EBP frame and saves the registers with `pushal`, so the
 * module builds without `-of+`.  The function holds no floating-point
 * instruction, so it needs no FPU stub and no assembly.
 */
extern int DAT_0016000d;
extern int DAT_00166688;
extern int DAT_0016669a;

extern int cd_audio_disc_info_request(void);
extern int cd_audio_q_channel_read(void);
extern void cd_audio_table_build(void);
extern void sound_driver_set(int value);

void cd_audio_tick(void)
{
    int status;

    DAT_0016000d = DAT_0016000d + 1;
    if (DAT_0016000d < 4) {
        status = cd_audio_q_channel_read();
        if ((unsigned char)status == 2 || (unsigned char)status == 0x0f) {
            DAT_00166688 = 1;
        }
        return;
    }
    DAT_0016000d = 0;
    status = cd_audio_disc_info_request();
    if ((unsigned char)status == 0xff) {
        DAT_00166688 = 1;
        if (DAT_0016669a != 0) {
            sound_driver_set(0);
        }
        return;
    }
    if ((DAT_00166688 & 1) == 0) {
        return;
    }
    status = cd_audio_q_channel_read();
    if ((unsigned char)status != 0) {
        return;
    }
    DAT_00166688 = 0;
    cd_audio_table_build();
    if (DAT_0016669a != 0) {
        sound_driver_set(0);
    }
}
