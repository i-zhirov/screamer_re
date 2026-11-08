/* The rebuilt `name_entry_screen` module.
 *
 * The original sits at linear 0x2c430 in STARTH.EXE and holds 230 instructions
 * and 885 bytes up to the `ret` at 0x2c7a4.  The record keeps the 888-byte
 * extent and sets no function size.
 *
 * The only caller is the state init dispatcher at 0x2be94, case 4, at 0x2bebe.
 * The matching state update is case 4 of `race_state_update` at 0x29564, which
 * calls 0x2ce84.  That dispatcher is a `race_frame` callee at 0x2807c, so the
 * module takes the section `race_frame callees`.
 *
 * The state init loads `menu2h.pcx` from `gfx\graf.dat` with the palette at
 * object-3 offset 0xcf5c, presents the buffer, copies 0x3e800 bytes to
 * `DAT_00121528`, fills the screen, and presents it again.  It clears
 * `DAT_0011d240` and `DAT_0011d244`, sets `DAT_00120378` at 1.5 and
 * `DAT_00120380` at 8.0, and sets `DAT_0011fac8` at 1.
 *
 * The first byte of the player name `DAT_001203cc` minus 0x41 becomes the
 * letter index `DAT_0011d23c`.  The input at 0x2893a increases that index,
 * wraps it above 0x19, and writes the letter `index + 0x41` into the name
 * buffer at position `DAT_0011d244`.  The letter count 26 and the A to Z
 * entries below support the name `name_entry_screen`.
 *
 * A non-zero index places the menu marker from the cosine and the sine of
 * `index * -0.24166098` at the scale 4500.0 and stores the heading
 * `index * 0.24166098` at `DAT_00120388` and `DAT_00120394`.  A zero index
 * clears the marker and stores the default 0x1194 at `DAT_001203b0` and
 * `DAT_001203a4`.  The function then stores the fixed values at
 * `DAT_001203ac`, `DAT_001203a0`, `DAT_00120384`, `DAT_0012038c`, and
 * `DAT_0012037c`, and converts the heading to `DAT_00109a0`.
 *
 * The loop builds 26 map object records at `DAT_001db78`, stride 0x64.  Each
 * record holds the letters `0x41 + i` and `0x21 + i`, the ring position at
 * radius 3500.0, the delay `i * 0.04`, and the angle `i * 0.24166098`.  The
 * `map_object_update` module at 0x2b9cc reads the same record fields.
 *
 * The original calls the cosine helper at 0x4da66, the sine helper at 0x4da72,
 * and the truncate helper at 0x4d9f4.  This harness links no math library, so
 * the local FPU stub supplies `cos` and `sin` from the hardware and makes
 * `__CHP` truncate toward zero.  The constants arrive through globals, because
 * the minimal image does not load the constant segment.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_0011d240;
extern float DAT_00120378;
extern float DAT_00120380;
extern int DAT_0011d244;
extern int DAT_0011d23c;
extern int DAT_0011fac8;
extern unsigned char DAT_001203cc[];
extern void *DAT_0011fad0;
extern void *DAT_00121528;
extern unsigned char DAT_0010cf5c[];

extern float DAT_00120388;
extern float DAT_00120394;
extern float DAT_00120384;
extern float DAT_0012038c;
extern float DAT_0012037c;
extern int DAT_0012039c;
extern int DAT_001203a0;
extern int DAT_001203a4;
extern int DAT_001203a8;
extern int DAT_001203ac;
extern int DAT_001203b0;
extern int DAT_00109a0;
extern int DAT_00109a8;

extern float DAT_00118e08;
extern int DAT_000007f9;
extern char DAT_001db78[];

extern float DAT_0010116c;
extern float DAT_00101170;
extern float DAT_00101174;
extern float DAT_00101178;
extern float DAT_0010117c;
extern float DAT_00101180;
extern double DAT_00101184;

extern int pcx_load(char *file, char *member, void *pixels, char *palette);
extern void sub_15150(void);
extern void video_present(float scale, int offset);
extern void *memcpy(void *target, const void *source, unsigned int count);
extern void video_fill(void);
extern void sub_2dff4(void);
extern void race_heading_update(void);
extern double sin(double value);
extern double cos(double value);

void name_entry_screen(void)
{
    char *record;
    float cos_value;
    float sin_value;
    float x;
    float y;
    float zero;
    float local160;
    float local3500;
    int index;
    int i;

    DAT_0011d240 = 0;
    DAT_00120378 = 1.5f;
    DAT_00120380 = 8.0f;
    DAT_0011d244 = 0;
    index = (int)DAT_001203cc[0] - 0x41;
    DAT_0011d23c = index;
    DAT_0011fac8 = 1;

    if (pcx_load((char *)0x115f, (char *)0x1154, DAT_0011fad0,
            (char *)0xcf5c) != 0) {
        sub_15150();
    }
    video_present(0.0f, 0);
    memcpy(DAT_00121528, DAT_0011fad0, 0x3e800);
    video_fill();
    video_present(0.0f, 0);

    if (index != 0) {
        cos_value = (float)cos((double)index * (double)DAT_0010116c);
        sin_value = (float)sin((double)index * (double)DAT_0010116c);
        DAT_001203a8 = (int)((double)sin_value * (double)DAT_00101170);
        DAT_0012039c = DAT_001203a8;
        DAT_001203b0 = (int)((double)cos_value * (double)DAT_00101170);
        DAT_00120388 = (float)((double)index * (double)DAT_00101174);
        DAT_00120394 = DAT_00120388;
        DAT_001203a4 = DAT_001203b0;
    } else {
        DAT_00120388 = 0.0f;
        DAT_001203a8 = 0;
        DAT_0012039c = 0;
        DAT_00120394 = 0.0f;
        DAT_001203b0 = 0x1194;
        DAT_001203a4 = 0x1194;
    }

    DAT_001203ac = 0x708;
    DAT_001203a0 = 0x708;
    DAT_00120384 = -0.6283185482025146f;
    DAT_0012038c = 0.0f;
    DAT_00109a0 = (int)((double)DAT_00120388 * (double)DAT_00101178
        / (double)DAT_0010117c) & 0x7ff;
    DAT_00109a8 = 0;
    DAT_0012037c = 0.0f;
    sub_2dff4();

    zero = 0.0f;
    local160 = 160.0f;
    local3500 = 3500.0f;
    x = zero;
    y = local3500;
    for (i = 0; i < 26; ++i) {
        record = DAT_001db78 + i * 0x64;
        *(int *)(record + 8) = i + 0x21;
        *(int *)(record + 4) = i + 0x41;
        *(int *)(record + 0x34) = (int)x;
        *(int *)(record + 0x40) = *(int *)(record + 0x34);
        *(int *)(record + 0x38) = (int)local160;
        *(int *)(record + 0x44) = (int)((double)((i & 1) * 500)
            + (double)local160 * (double)DAT_00101180);
        *(int *)(record + 0x3c) = (int)local3500;
        *(int *)(record + 0x48) = *(int *)(record + 0x3c);
        *(int *)(record + 0x10) = 0x400cbe4d;
        *(float *)(record + 0x14) =
            (float)((double)i * (double)DAT_00101174);
        *(int *)(record + 0x18) = 0;
        *(int *)(record + 0xc) = 1;
        *(int *)(record + 0x58) = 0;
        *(float *)(record + 0x5c) = 1.0f;
        *(float *)(record + 0x60) = (float)((double)i * DAT_00101184);
        cos_value = (float)cos((double)(i + 1) * (double)DAT_0010116c);
        sin_value = (float)sin((double)(i + 1) * (double)DAT_0010116c);
        x = (float)((double)local3500 * (double)sin_value
            + (double)zero * (double)cos_value);
        y = (float)((double)local3500 * (double)cos_value
            - (double)zero * (double)sin_value);
    }

    DAT_00118e08 = 0.08f;
    DAT_000007f9 = 8;
    race_heading_update();
}
