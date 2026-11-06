/* The test harness for the rebuilt `video_present` module.
 *
 * The harness holds the 0x300 source bytes and the palette stub.  The stub
 * copies the palette before the module returns.  The harness runs three scale
 * and offset cases:
 *
 *     0  the scale 1.0 and the offset 0 copy the source bytes
 *     1  the scale 0.5 halves the even source bytes
 *     2  the scale 2.0 and the offset 10 lift the byte 20 to 30
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void video_present(float scale, int offset);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_0010cf5c[0x300];

static char text[8];
static unsigned char got_palette[0x300];
static int palette_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void palette_set(char *palette)
{
    int index;

    for (index = 0; index < 0x300; ++index) {
        got_palette[index] = palette[index];
    }
    palette_calls = palette_calls + 1;
}

int main(void)
{
    int index;

    for (index = 0; index < 0x300; ++index) {
        DAT_0010cf5c[index] = (unsigned char)index;
    }
    palette_calls = 0;
    video_present(1.0f, 0);
    put(palette_calls == 1 && got_palette[0] == 0 && got_palette[1] == 1
        && got_palette[0xff] == 0xff && got_palette[0x2ff] == 0xff ? '0' : 'x');

    for (index = 0; index < 0x300; ++index) {
        DAT_0010cf5c[index] = (unsigned char)(index * 2);
    }
    palette_calls = 0;
    video_present(0.5f, 0);
    put(palette_calls == 1 && got_palette[0] == 0 && got_palette[1] == 1
        && got_palette[0xff] == 0x7f ? '1' : 'x');

    DAT_0010cf5c[0] = 20;
    palette_calls = 0;
    video_present(2.0f, 10);
    put(palette_calls == 1 && got_palette[0] == 30 ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
