/* The test harness for the rebuilt `default_data_copy` module.
 *
 * The harness fills the source block with a pattern, clears the target block,
 * calls the module, and checks the target.  The module copies through
 * `rep movsd`, which reads through DS:ESI and writes through ES:EDI.  The
 * harness start code `_cstart_` in tools/rebuild/chk.asm sets ES to DS, so the
 * write reaches the target block.
 *
 *     V  the target holds the source pattern
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void default_data_copy(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_00010113[32];
unsigned int DAT_000f0002[32];

static char text[8];

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
    unsigned i;
    unsigned ok;

    for (i = 0; i < 32; i = i + 1) {
        DAT_00010113[i] = (i * 0x100) + 1;
        DAT_000f0002[i] = 0;
    }
    default_data_copy();
    ok = 1;
    for (i = 0; i < 32; i = i + 1) {
        if (DAT_000f0002[i] != DAT_00010113[i]) {
            ok = 0;
        }
    }
    put(ok ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
