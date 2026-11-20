/* The rebuilt `cd_audio_resume` module.
 *
 * The original sits at linear 0x264f8 in STARTH.EXE and holds 14 instructions
 * and 46 bytes up to the `ret` at 0x26525.  The next recorded function starts
 * at 0x265b4, so the store record keeps the 188-byte extent and sets no
 * separate function size.  The identity check uses 14 instructions.
 *
 * The only direct caller is `options_screen` at 0x21338, at the call site
 * 0x219be.  The caller chooses this function when the saved pair flag at
 * `+0x4a` is set, and it chooses `cd_audio_play` at 0x263cc otherwise.  The CD
 * audio request path sits in the section race_loop callees, so the module takes
 * the same section.
 *
 * The function returns at once when bit 0 of `DAT_0016669e` is clear or bit 0
 * of `DAT_00166684` is clear.  Otherwise it builds the MSCDEX request header at
 * `DAT_001666a4`: the length 0x1a at +0, the byte 0x88 at +1, and the command
 * 0x88 at +2.  It then runs the request through `0x25f44`.
 *
 * DOSBox maps the driver command 0x88 to Resume Audio.  The caller flag, the
 * request, and the command support the name `cd_audio_resume`.  The original
 * has no symbol, so the record keeps the name with the confidence inferred.
 *
 * The original has no EBP frame and saves the registers with `pushal`, so the
 * module builds without `-of+`.  The function holds no floating-point
 * instruction, so it needs no FPU stub and no assembly.
 */
extern unsigned int DAT_0016669e;
extern unsigned int DAT_00166684;
extern unsigned int DAT_001666a4;

extern void cd_request_send(void);

void cd_audio_resume(void)
{
    unsigned char *request;

    if ((DAT_0016669e & 1) == 0) {
        return;
    }
    if ((DAT_00166684 & 1) == 0) {
        return;
    }
    request = (unsigned char *)DAT_001666a4;
    request[0] = 0x1a;
    request[2] = 0x88;
    request[1] = 0x88;
    cd_request_send();
}
