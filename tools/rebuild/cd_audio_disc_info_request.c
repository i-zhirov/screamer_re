/* The rebuilt `cd_audio_disc_info_request` module.
 *
 * The original sits at linear 0x260a0 in STARTH.EXE.  The body holds 22
 * instructions and 62 bytes up to the `ret` at 0x260dd.  Two padding bytes at
 * 0x260de precede the next function at 0x260e0, so the store record keeps the
 * 64-byte extent and sets no separate function size.  The identity check uses
 * 22 instructions.
 *
 * The direct call sites are 0x25e82 and 0x25eb3 in the CD audio setup at
 * 0x25dd5, 0x25fa8 in the CD audio tick at 0x25f90, 0x26169 in the playback
 * block at 0x26148, and 0x2630c in cd_audio_table_build at 0x262fc.  The CD
 * audio request path sits in the section race_loop callees, so the module takes
 * the same section.
 *
 * The function builds the MSCDEX request header at `DAT_001666a4`: the length
 * 0x1a, the subunit `DAT_00166680`, the command 3 (IOCTL input), the transfer
 * count 2, and the payload subfunction 9 (get audio disc info).  It clears the
 * dword at +0x41 and the dword at +0x45, runs the request through the helper at
 * 0x25f24, and returns the dword at the response pointer `DAT_001666a8`.
 *
 * The setup at 0x25dd5 sets the response pointer `DAT_001666a8` to the request
 * plus 0x41, so the return is the first dword of the disc info payload.  The CD
 * audio tick tests the low byte for 0xff, so the return marks a missing disc.
 *
 * The original has no EBP frame and saves the registers with `pushal`, so the
 * module builds without `-of+`.  The function holds no floating-point
 * instruction, so it needs no FPU stub and no assembly.
 */
extern unsigned int DAT_001666a4;
extern unsigned int DAT_001666a8;
extern unsigned char DAT_00166680;

extern void cd_request_submit(void);

int cd_audio_disc_info_request(void)
{
    unsigned char *request;

    request = (unsigned char *)DAT_001666a4;
    request[0] = 0x1a;
    request[2] = 3;
    request[1] = DAT_00166680;
    *(unsigned short *)(request + 0x12) = 2;
    request[0x40] = 9;
    *(unsigned int *)(request + 0x41) = 0;
    *(unsigned int *)(request + 0x45) = 0;
    cd_request_submit();
    return (int)*(unsigned int *)DAT_001666a8;
}
