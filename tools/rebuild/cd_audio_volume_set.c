/* The rebuilt `cd_audio_volume_set` module.
 *
 * The original sits at linear 0x265b4 in STARTH.EXE.  The body holds 43
 * instructions and 132 bytes up to the `ret` at 0x26637.  The shared guard tail
 * at 0x26638 and the next recorded function at 0x2666c give the 184-byte
 * extent, so the store record keeps that extent and sets no separate function
 * size.  The identity check uses 43 instructions.
 *
 * The only direct caller is `view_move` at 0x1f94c, which calls this function
 * at 0x1f984 with the same shifted view value in `eax` and `edx`.  The caller
 * sits in the section motion_update callees, so this module takes the same
 * section.  The caller applies the two audio channels with the same value.
 *
 * The two state flags `DAT_0016669e` bit 0 and `DAT_00166684` bit 0 gate the
 * call.  A repeated left and right pair returns at once, so the driver sees no
 * duplicate request.  The function then builds the MSCDEX request header at
 * `DAT_001666a4`: the length 0x1a, the subunit `DAT_00166680`, the command 0x0c
 * (IOCTL output), and the transfer count 9.
 *
 * The 9-byte payload at +0x40 is the audio channel control subfunction 3.  The
 * output channels 0 and 1 take the input sources 0 and 1, and their volumes are
 * the low bytes of the two arguments, with a zero value raised to 1.  The two
 * extra channels stay zero.  The request runs through 0x25f24.
 *
 * The original has no EBP frame and saves the registers with `pushal`, so the
 * module builds without `-of+`.  The function holds no floating-point
 * instruction, so it needs no FPU stub and no assembly.
 */
extern int DAT_0016669e;
extern int DAT_00166684;
extern int DAT_001666b4;
extern int DAT_001666b0;
extern unsigned int DAT_001666a4;
extern unsigned char DAT_00166680;

extern void cd_request_submit(void);

int cd_audio_volume_set(int left, int right)
{
    unsigned char *request;
    unsigned int left_volume;
    unsigned int right_volume;

    if ((DAT_0016669e & 1) == 0) {
        return 1;
    }
    if ((DAT_00166684 & 1) == 0) {
        return 1;
    }
    if (left == DAT_001666b4 && right == DAT_001666b0) {
        return 1;
    }
    DAT_001666b4 = left;
    DAT_001666b0 = right;
    request = (unsigned char *)DAT_001666a4;
    request[0] = 0x1a;
    request[2] = 0x0c;
    request[1] = DAT_00166680;
    *(unsigned short *)(request + 0x12) = 9;
    request[0x40] = 3;
    right_volume = right ? (unsigned int)right : 1;
    right_volume = (right_volume << 8) | 1;
    *(unsigned short *)(request + 0x43) = (unsigned short)right_volume;
    left_volume = left ? (unsigned int)left : 1;
    left_volume = left_volume << 8;
    *(unsigned short *)(request + 0x41) = (unsigned short)left_volume;
    *(unsigned int *)(request + 0x45) = 0;
    cd_request_submit();
    return left;
}
