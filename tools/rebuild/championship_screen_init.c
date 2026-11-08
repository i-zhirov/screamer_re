extern int DAT_0011d240;
extern float DAT_00120378;
extern float DAT_00120380;
extern int DAT_0011fac8;
extern int DAT_001201c0[];
extern int DAT_0011d260;
extern int DAT_00120400[];
extern int DAT_001203dc;
extern int DAT_001203e0;
extern char DAT_001022bc[];
extern void *DAT_0011fad0;
extern void *DAT_00121528;
extern unsigned char DAT_0010cf5c[];
extern float DAT_00118e08;
extern int DAT_000007f9;
extern int DAT_0011d24c;
extern char championship_format[];
extern char championship_archive[];

extern char *DAT_00120030[];
extern int sprintf(char *buffer, const char *format, ...);
extern int pcx_load(char *file, char *member, void *pixels, char *palette);
extern void sub_15150(void);
extern void palette_shade_build(unsigned char *palette);
extern void video_present(float scale, int offset);
extern void *memcpy(void *target, const void *source, unsigned int count);
extern void video_fill(void);
extern void sub_2dff4(void);

void championship_screen_init(void)
{
    char member[0x80];
    char *cursor;
    char *record_name;
    int i;
    int start;
    int count;

    DAT_0011d240 = 1;
    DAT_00120378 = 1.5f;
    DAT_00120380 = 0.95f;
    DAT_0011fac8 = 1;
    sub_2dff4();

    i = 0;
    start = 0;
    count = 0;
    while (DAT_00120030[i] != (char *)-1) {
        for (;;) {
            cursor = DAT_00120030[i];
            while (*cursor != '#' && *cursor != 0) {
                cursor++;
            }
            if (*cursor == '#') {
                break;
            }
            i++;
        }
        DAT_001201c0[count] = start;
        count++;
        i++;
        start = i;
    }
    DAT_001201c0[count] = i;

    if (DAT_0011d260 == -1) {
        for (i = 0; i < 6; i++) {
            DAT_00120400[i] = -1;
        }
        DAT_001203dc = 0;
        DAT_001203e0 = 0;
    }

    record_name = DAT_001022bc + 0x18 + DAT_001203dc * 0x2c;
    sprintf(member, championship_format, record_name, 'H');
    if (pcx_load(championship_archive, member, DAT_0011fad0,
            (char *)DAT_0010cf5c) != 0) {
        sub_15150();
    }
    DAT_00118e08 = 0.08f;
    DAT_000007f9 = 8;
    palette_shade_build(DAT_0010cf5c);
    video_present(0.0f, 0);
    memcpy(DAT_00121528, DAT_0011fad0, 0x3e800);
    video_fill();
    video_present(0.0f, 0);
    DAT_0011d24c = 0;
}
