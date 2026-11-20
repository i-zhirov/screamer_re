/* The rebuilt `cd_request_submit` module.
 *
 * The original sits at linear 0x25f24 in STARTH.EXE.  The body holds 11
 * instructions and 31 bytes up to the `ret` at 0x25f42.  The next function
 * starts at 0x25f44, so the store record keeps the 32-byte extent and sets no
 * separate function size.  The identity check uses 11 instructions.
 *
 * The function receives the request header address in `edi`.  It forms the
 * real-mode address of the transfer buffer at request + 0x40 and writes the
 * pair into the MSCDEX header: the low nibble at +0x0e and the shifted high
 * part at +0x10.  It then calls `cd_request_send` at 0x25f44, which submits the
 * block through the DPMI INT 2Fh path.
 *
 * The direct callers are the MSCDEX request builders at 0x260a0, 0x260e0,
 * 0x26120, 0x261a8, 0x261e4, 0x263cc, 0x264b8, 0x264f8, and 0x265b4.  They all
 * build the request header at DAT_001666a4, put the payload at +0x40, and then
 * submit it here.  The CD request path sits in the section race_loop callees,
 * so the record takes the same section.
 *
 * The request header, the transfer buffer, and the shared sender support the
 * name `cd_request_submit`.  The original has no symbol, so the record keeps
 * the name with the confidence inferred.
 *
 * The original has no EBP frame and saves no register, so the module builds
 * without `-of+`.  The function holds no floating-point instruction, so it
 * needs no FPU stub.  The module uses no assembly.
 */
extern int cd_request_send(unsigned int request);

int cd_request_submit(unsigned int request)
{
    unsigned int high;
    unsigned int low;

    high = (request + 0x40) >> 4;
    low = (request + 0x40) & 0xf;
    *(unsigned short *)(request + 0x10) = (unsigned short)high;
    *(unsigned short *)(request + 0xe) = (unsigned short)low;
    return cd_request_send(request);
}
