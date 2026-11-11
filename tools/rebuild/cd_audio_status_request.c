/* The rebuilt `cd_audio_status_request` module.
 *
 * The original sits at linear 0x260e0 in STARTH.EXE.  The body holds 22
 * instructions and 64 bytes up to the `ret` at 0x2611f.  The next function
 * starts at 0x26120, so the store record keeps the 64-byte extent and sets no
 * separate function size.  The identity check uses 22 instructions.
 *
 * The only direct caller is `input_poll` at 0x26448, at the call site 0x2647e.
 * The caller reads the status word at +3 and tests bit 9 (busy), so the
 * request sits in the section race_loop callees.
 *
 * The function builds the MSCDEX request header at `DAT_001666a4`: the length
 * 0x1a, the subunit `DAT_00166680`, the command 3 (IOCTL input), the transfer
 * count 0x0b, and the payload subfunction 0x0f (get audio status).  It clears
 * the dword at +0x41 and the dword at +0x45, runs the request through the
 * helper at 0x25f24, and returns the raw status word at +3.
 *
 * The MSCDEX subfunction table gives the 0x0f code the 11-byte audio status
 * block.  The caller, the request, and the returned status support the name
 * `cd_audio_status_request`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has no EBP frame and saves the registers with `pushal`, so the
 * module builds without `-of+`.  The function holds no floating-point
 * instruction, so it needs no FPU stub and no assembly.
 */
extern unsigned int DAT_001666a4;
extern unsigned char DAT_00166680;

extern void cd_request_submit(void);

int cd_audio_status_request(void)
{
    unsigned char *request;

    request = (unsigned char *)DAT_001666a4;
    request[0] = 0x1a;
    request[2] = 3;
    request[1] = DAT_00166680;
    *(unsigned short *)(request + 0x12) = 0x0b;
    request[0x40] = 0x0f;
    *(unsigned int *)(request + 0x41) = 0;
    *(unsigned int *)(request + 0x45) = 0;
    cd_request_submit();
    request = (unsigned char *)DAT_001666a4;
    return (int)*(unsigned short *)(request + 3);
}
