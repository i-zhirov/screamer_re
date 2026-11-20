/* The rebuilt `cd_audio_track_query` module.
 *
 * The original sits at linear 0x261a8 in STARTH.EXE.  The identity window holds
 * 21 instructions and 57 bytes up to the ret at 0x261e0.  The next recorded
 * function starts at 0x261e4 after the three padding bytes at 0x261e1, so the
 * store record keeps the 60-byte extent and sets no separate function size.
 * The identity check uses 21 instructions.
 *
 * The only direct caller is `cd_audio_table_build` at 0x262fc, call site
 * 0x2634d.  The caller passes the track number in eax and reads the response
 * control byte at +0x46 bit 6 after the call to tell a data track from an audio
 * track.  The caller sits in the section race_loop callees, so this module
 * takes the same section.
 *
 * The function builds the MSCDEX request header at `DAT_001666a4`: the length
 * 0x1a, the subunit `DAT_00166680`, the command 3 (IOCTL input), and the
 * transfer count 7.  It writes the payload subfunction 0x0b (audio track info),
 * the input track number at +0x41, and two zero dwords at +0x42 and +0x46.  The
 * request runs through 0x25f24.
 *
 * The MSCDEX subfunction table gives the 0x0b code the per-track audio track
 * info record, and the caller reads the returned control byte.  The request and
 * the caller support the name `cd_audio_track_query`.  The original has no
 * symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has no EBP frame and saves the registers with `pushal`, so the
 * module builds without `-of+`.  The function holds no floating-point
 * instruction, so it needs no FPU stub and no assembly.
 */
extern unsigned int DAT_001666a4;
extern unsigned char DAT_00166680;

extern void cd_request_submit(void);

void cd_audio_track_query(unsigned char track)
{
    unsigned char *request;

    request = (unsigned char *)DAT_001666a4;
    request[0] = 0x1a;
    request[2] = 3;
    request[1] = DAT_00166680;
    *(unsigned short *)(request + 0x12) = 7;
    request[0x40] = 0x0b;
    request[0x41] = track;
    *(unsigned int *)(request + 0x42) = 0;
    *(unsigned int *)(request + 0x46) = 0;
    cd_request_submit();
}
