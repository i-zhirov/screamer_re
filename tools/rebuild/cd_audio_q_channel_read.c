/* The rebuilt `cd_audio_q_channel_read` module.
 *
 * The original sits at linear 0x261e4 in STARTH.EXE.  The body holds 25
 * instructions and 74 bytes up to the `ret` at 0x2622d.  A clear status bit
 * jumps to the shared tail at 0x2622e, and the next recorded function starts at
 * 0x262a8, so the store record keeps the 196-byte extent and sets no separate
 * function size.  The identity check uses 25 instructions.
 *
 * The direct callers are the CD audio tick at 0x25f90, the playback start at
 * 0x26148, and the options screen at 0x21338.  The tick calls this function
 * every fourth frame and compares the return byte with 2 and 0x0f.  The options
 * screen sits in the section race_loop callees, so the module takes the same
 * section.
 *
 * The function builds the MSCDEX request header at `DAT_001666a4`: the length
 * 0x1a, the subunit `DAT_00166680`, the command 3 (IOCTL input), and the
 * transfer count 0x0b.  The payload subfunction 0x0c is the audio Q-channel
 * info.  The 11-byte response at +0x41 is zero first.  The request runs through
 * 0x25f24.  The function then reads the status word at +3 and returns 0 when
 * the error bit 15 is clear, or the status word when the error bit is set.
 *
 * The MSCDEX subfunction table gives the 0x0c code the 11-byte control block of
 * the audio Q-channel info, and the request holds that transfer count.  The
 * request and the return support the name `cd_audio_q_channel_read`.  The
 * original has no symbol, so the record keeps the name with the confidence
 * inferred.
 *
 * The original has no EBP frame and saves the registers with `pushal`, so the
 * module builds without `-of+`.  The function holds no floating-point
 * instruction, so it needs no FPU stub and no assembly.
 */
extern unsigned int DAT_001666a4;
extern unsigned char DAT_00166680;

extern void cd_request_submit(void);

int cd_audio_q_channel_read(void)
{
    unsigned char *request;
    unsigned short status;

    request = (unsigned char *)DAT_001666a4;
    request[0] = 0x1a;
    request[2] = 3;
    request[1] = DAT_00166680;
    *(unsigned short *)(request + 0x12) = 0x0b;
    request[0x40] = 0x0c;
    *(unsigned int *)(request + 0x41) = 0;
    *(unsigned int *)(request + 0x45) = 0;
    *(unsigned int *)(request + 0x49) = 0;
    cd_request_submit();
    request = (unsigned char *)DAT_001666a4;
    status = *(unsigned short *)(request + 3);
    if ((status & 0x8000) == 0) {
        return 0;
    }
    return (int)status;
}
