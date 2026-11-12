/* The rebuilt `cd_audio_play` module.
 *
 * The original sits at linear 0x263cc in STARTH.EXE.  The first block holds 13
 * instructions and 49 bytes up to the `ret` at 0x263fc.  A branch at 0x263f4
 * enters the second block at 0x263fd, and that block ends with the `ret` at
 * 0x26447.  The next recorded function starts at 0x26448, so the store record
 * keeps the 124-byte extent and sets no separate function size.  The identity
 * check uses 13 instructions.
 *
 * The callers at 0x12b74, 0x219b7, 0x29f3b, and 0x264a8 pass a track number in
 * `eax` and the value 1 in `edx`.  The sibling functions at 0x264b8, 0x263cc,
 * and 0x26528 send the stop, play, and seek commands through the same request
 * block, so the function sends the play-audio request.  The caller set and the
 * table reader support the name `cd_audio_play`.  The original has no symbol,
 * so the record keeps the name with the confidence inferred.
 *
 * The function returns at once when bit 0 of `DAT_0016669e` is clear or bit 0
 * of `DAT_00166684` is clear.  Otherwise it stores the track at
 * `DAT_00166696` and the flag at `DAT_001666ac`.  A track below the first
 * track `DAT_0016668e` or above the last track `DAT_00166692` returns 1.
 *
 * A track inside the range selects the 8-byte entry of the table at
 * `DAT_001666c0`: the start time and the duration.  The function builds the
 * MSCDEX request header at `DAT_001666a4` with the length 0x16, the subunit
 * `DAT_00166680`, the command 0x84 (play audio), and the zero byte at +0x0d.
 * The start time goes to +0x0e and the duration to +0x12.  The request runs
 * through the send helper at 0x25f44, and the function returns 0.
 *
 * The original has no EBP frame and saves the registers with `pushal`, so the
 * module builds without `-of+`.  The function holds no floating-point
 * instruction, so it needs no FPU stub and no assembly.
 */
extern int DAT_0016669e;
extern int DAT_00166684;
extern int DAT_00166696;
extern int DAT_001666ac;
extern int DAT_0016668e;
extern int DAT_00166692;
extern int DAT_001666c0[];
extern unsigned int DAT_001666a4;
extern unsigned char DAT_00166680;

extern void cd_request_send(void);

int cd_audio_play(int track, int flag)
{
    unsigned char *request;
    int *entry;
    int first;

    if ((DAT_0016669e & 1) == 0) {
        return 1;
    }
    if ((DAT_00166684 & 1) == 0) {
        return 1;
    }
    DAT_00166696 = track;
    DAT_001666ac = flag;
    first = DAT_0016668e;
    if (track < first) {
        return 1;
    }
    if (track > DAT_00166692) {
        return 1;
    }
    entry = DAT_001666c0 + (track - first) * 2;
    request = (unsigned char *)DAT_001666a4;
    request[0] = 0x16;
    request[2] = 0x84;
    request[1] = DAT_00166680;
    request[0x0d] = 0;
    *(unsigned int *)(request + 0x0e) = (unsigned int)entry[0];
    *(unsigned int *)(request + 0x12) = (unsigned int)entry[1];
    cd_request_send();
    return 0;
}
