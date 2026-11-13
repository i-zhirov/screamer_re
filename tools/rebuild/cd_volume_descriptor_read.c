/* The rebuilt `cd_volume_descriptor_read` module.
 *
 * The original sits at linear 0x26014 in STARTH.EXE.  The plan row holds the
 * first 16 bytes and 7 instructions up to the `ret` at 0x26023.  The nonzero
 * path continues in the block at 0x26024 and ends with the `ret` at 0x26067,
 * and the second request builder starts at 0x26068, so the store record keeps
 * the 140-byte extent and sets no separate function size.  The identity check
 * uses 7 instructions.
 *
 * The body first tests the word `DAT_0016668c`.  The setup at 0x25dd5 stores
 * the MSCDEX start drive letter there.  A zero word returns 0.  Otherwise the
 * function builds the MSCDEX read-long request header at `DAT_001666a4`: the
 * length 0x1a, the subunit `DAT_00166680`, the command 0x80 (read long), the
 * addressing mode 0, the sector count 1, the start sector 0x10, and the
 * reserved dword 0.  It runs the request through the helper at 0x25f24 and
 * returns the response buffer at `DAT_001666a4` plus 0x40.
 *
 * The start sector 0x10 is the ISO 9660 primary volume descriptor.  The caller
 * `cd_check` at 0x157b8 reads the system identifier at +8, the volume
 * identifier at +0x28, the publisher identifier at +0x13e, and the application
 * identifier at +0x23e.  The disc holds `PC`, `SCREAMER`, `VIE`, and
 * `SCREAMER` at those offsets, so the function reads the volume descriptor.
 *
 * The original has no EBP frame and saves the registers with `pushal`, so the
 * module builds without `-of+`.  The function holds no floating-point
 * instruction, so it needs no FPU stub and no assembly.
 */
extern unsigned short DAT_0016668c;
extern unsigned int DAT_001666a4;
extern unsigned char DAT_00166680;

extern void cd_request_submit(void);

void *cd_volume_descriptor_read(void)
{
    unsigned char *request;

    if (DAT_0016668c == 0) {
        return 0;
    }
    request = (unsigned char *)DAT_001666a4;
    request[0] = 0x1a;
    request[2] = 0x80;
    request[1] = DAT_00166680;
    request[0x0d] = 0;
    *(unsigned short *)(request + 0x12) = 1;
    *(unsigned int *)(request + 0x14) = 0x10;
    *(unsigned int *)(request + 0x18) = 0;
    cd_request_submit();
    return (char *)DAT_001666a4 + 0x40;
}
