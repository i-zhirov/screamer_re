/* The test harness for the rebuilt `stream_layer_init` module.
 *
 * The harness holds the two state words and the six setup stubs.  Each stub
 * counts its calls.  The harness checks the state and the call set:
 *
 *     0  the stream pointer is stored and the state word is clear
 *     1  the six setup stubs ran once each
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void stream_layer_init(void *stream);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_0002153c;
void *DAT_00021574;

static char text[8];
static int calls[6];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void step(int index)
{
    calls[index] = calls[index] + 1;
}

void stream_setup_0(void) { step(0); }
void stream_setup_1(void) { step(1); }
void stream_setup_2(void) { step(2); }
void persptab_load(void) { step(3); }
void vis_tab_load(void) { step(4); }
void stream_setup_5(void) { step(5); }

int main(void)
{
    unsigned char stream_obj[8];
    unsigned i;
    int ok;

    DAT_0002153c = 0xffffffff;
    DAT_00021574 = 0;
    for (i = 0; i < 6; i = i + 1) {
        calls[i] = 0;
    }
    stream_layer_init(stream_obj);
    ok = DAT_00021574 == stream_obj && DAT_0002153c == 0;
    for (i = 0; i < 6; i = i + 1) {
        if (calls[i] != 1) {
            ok = 0;
        }
    }
    put(ok ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
