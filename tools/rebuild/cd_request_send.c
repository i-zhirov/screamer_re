/* The rebuilt `cd_request_send` module.
 *
 * The original sits at linear 0x25f44 in STARTH.EXE.  The body holds 20
 * instructions and 74 bytes up to the `ret` at 0x25f8d.  The next function
 * starts at 0x25f90, so the store record keeps the 76-byte extent and sets no
 * separate function size.  The identity check uses 20 instructions.
 *
 * The direct callers are the request helper at 0x25f24 at the call site
 * 0x25f3d, the playback block at 0x263cc at the call site 0x2643f, and the
 * request helpers at 0x264b8 and 0x264f8 at the call sites 0x264e4, 0x2651f,
 * and 0x2657a.  The CD audio request path sits in the section race_loop
 * callees, so the module takes the same section.
 *
 * The function receives the request header address in `edi`.  It puts the low
 * nibble into the RMCS register word at `DAT_001176d0` and the shifted high
 * part into the segment word at `DAT_001176e2`, so the pair is the real-mode
 * far pointer `ES:BX` of the request header.  It sets the RMCS `eax` field to
 * 0x1510 (INT 2Fh send device driver request) and the `ecx` field to the CD
 * drive unit at `DAT_0016668c`.
 *
 * The function then runs INT 31h with `ax` 0x0300 (simulate real-mode
 * interrupt), `bl` 0x2f (the multiplex interrupt), and `es:edi` at the RMCS
 * 0x176c0.  It returns the status word at offset 3 of the global request
 * header at `DAT_001666a4`.  The request header, the drive unit, and the return
 * support the name `cd_request_send`.  The original has no symbol, so the
 * record keeps the name with the confidence inferred.
 *
 * The original has no EBP frame and saves no register, so the module builds
 * without `-of+`.  The module uses no assembly.  It calls the DPMI real-mode
 * interrupt through the harness stub `dpmi_simulate_real_int`, because the C
 * build cannot issue the INT 31h instruction itself.  The function holds no
 * floating-point instruction, so it needs no FPU stub.
 */
extern unsigned int DAT_001666a4;
extern unsigned short DAT_0016668c;
extern unsigned int DAT_001176d0;
extern unsigned int DAT_001176d8;
extern unsigned int DAT_001176dc;
extern unsigned short DAT_001176e2;

extern void dpmi_simulate_real_int(unsigned int vector, void *rmcs);

int cd_request_send(unsigned int request)
{
    unsigned int value;
    unsigned int high;

    value = request;
    high = request >> 4;
    DAT_001176e2 = (unsigned short)high;
    DAT_001176d0 = value & 0xf;
    DAT_001176dc = 0x1510;
    DAT_001176d8 = DAT_0016668c;
    dpmi_simulate_real_int(0x2f, (void *)0x176c0);
    request = DAT_001666a4;
    return (int)*(unsigned short *)(request + 3);
}
