/* The rebuilt `cd_audio_track_info_request` module.
 *
 * The original sits at linear 0x26120 in STARTH.EXE.  The body holds 13
 * instructions and 38 bytes up to the `jmp` at 0x26144.  The jump enters the
 * shared tail at 0x26104, which writes the payload subfunction, clears two
 * dwords, runs the DPMI request through 0x25f24, and returns the status word.
 * The plan row counts 22 instructions and 68 bytes through the first `ret` at
 * 0x26163.  The next recorded function starts at 0x261a8, so the store record
 * keeps the 136-byte extent and sets no separate function size.  The identity
 * check uses 22 instructions.
 *
 * The call sites are inside `cd_audio_table_build` at 0x262fc.  The function
 * builds the MSCDEX request header at `DAT_001666a4`: the length 0x1a, the
 * subunit `DAT_00166680`, the command 3 (IOCTL input), and the transfer count
 * 7.  The payload subfunction 0x0a is the audio track info.  The request runs
 * through 0x25f24, and the function returns the raw status word at +3.
 *
 * The MSCDEX subfunction table gives the 0x0a code the 7-byte audio track info
 * record, and the request holds that transfer count.  The request, the caller,
 * and the returned status support the name `cd_audio_track_info_request`.  The
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

int cd_audio_track_info_request(void)
{
    unsigned char *request;

    request = (unsigned char *)DAT_001666a4;
    request[0] = 0x1a;
    request[2] = 3;
    request[1] = DAT_00166680;
    *(unsigned short *)(request + 0x12) = 7;
    request[0x40] = 0x0a;
    *(unsigned int *)(request + 0x41) = 0;
    *(unsigned int *)(request + 0x45) = 0;
    cd_request_submit();
    request = (unsigned char *)DAT_001666a4;
    return (int)*(unsigned short *)(request + 3);
}
