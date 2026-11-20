/* The rebuilt `cd_audio_stop` module.
 *
 * The original sits at linear 0x264b8 in STARTH.EXE and holds 15 instructions
 * and 51 bytes up to the `ret` at 0x264ea.  The next function starts at
 * 0x264ec after the padding byte at 0x264eb, so the store record keeps the
 * 52-byte extent and sets no separate function size.  The identity check uses
 * 15 instructions.
 *
 * The only direct caller is `options_screen` at 0x21338, at the call site
 * 0x2146d.  The wrapper at 0x264ec calls this function twice.  The CD audio
 * request path sits in the section race_loop callees, so the module takes the
 * same section.
 *
 * The function returns at once when bit 0 of `DAT_0016669e` is clear or bit 0
 * of `DAT_00166684` is clear.  Otherwise it builds the MSCDEX request header at
 * `DAT_001666a4`: the length 0x0d, the subunit `DAT_00166680`, and the command
 * 0x85 (stop audio).  It then runs the request through `0x25f44`.
 *
 * DOSBox maps the driver command 0x85 to Stop Audio.  The two guard flags, the
 * request, and the command support the name `cd_audio_stop`.  The original has
 * no symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has no EBP frame and saves the registers with `pushal`, so the
 * module builds without `-of+`.  The function holds no floating-point
 * instruction, so it needs no FPU stub and no assembly.
 */
extern unsigned int DAT_0016669e;
extern unsigned int DAT_00166684;
extern unsigned int DAT_001666a4;
extern unsigned char DAT_00166680;

extern void cd_request_send(void);

void cd_audio_stop(void)
{
    unsigned char *request;

    if ((DAT_0016669e & 1) == 0) {
        return;
    }
    if ((DAT_00166684 & 1) == 0) {
        return;
    }
    request = (unsigned char *)DAT_001666a4;
    request[0] = 0x0d;
    request[2] = 0x85;
    request[1] = DAT_00166680;
    cd_request_send();
}
