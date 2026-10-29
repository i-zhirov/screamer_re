/* The test harness for the rebuilt `dpmi_real_int` module.
 *
 * The harness holds the state words and the error path stubs.  The module sets
 * the state flag `DAT_00116684` at the entry, runs the DOS multiplexer call,
 * and returns.  The harness checks the state flag, which the module always
 * sets:
 *
 *     V  the module ran and set the state flag
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void dpmi_real_int(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_00116684;
unsigned int DAT_00116688;
unsigned short DAT_0011668c;
unsigned int DAT_0011669e;
unsigned short DAT_001166a2;
unsigned int DAT_001166a4;
unsigned int DAT_001166a8;
unsigned char DAT_00116680;
unsigned char DAT_001176c0[0x200];
unsigned int DAT_001176d0;
unsigned int DAT_001176d8;
unsigned int DAT_001176dc;
unsigned short DAT_001176e2;

static char text[8];

/* The error path stubs of the module.  The harness links no run-time library,
 * so the stubs stand in for the report and the driver calls of the real-mode
 * interrupt path. */
int error_report(int code);
void drm_260a0(void);
void drm_260e0(void);
void drm_26014(void);
void drm_264ec(void);
void drm_262a8(void);
int drm_26120(void);
int drm_262fc(void);

int error_report(int code) { return 0; }
void drm_260a0(void) { }
void drm_260e0(void) { }
void drm_26014(void) { }
void drm_264ec(void) { }
void drm_262a8(void) { }
int drm_26120(void) { return 0; }
int drm_262fc(void) { return 0; }

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int main(void)
{
    DAT_00116684 = 0;
    dpmi_real_int();
    put(DAT_00116684 == 1 ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
