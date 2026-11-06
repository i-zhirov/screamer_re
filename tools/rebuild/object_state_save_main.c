/* The test harness for the rebuilt `object_state_save` module.
 *
 * The harness holds an object record, sets the five source fields to distinct
 * values, calls the module, and checks the five destination fields:
 *
 *     0  the four dword fields hold the four source values
 *     1  the float field holds the source bit pattern
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void object_state_save(char *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x4a4];

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
    int index;

    for (index = 0; index < 0x4a4; ++index) {
        object[index] = 0;
    }
    *(unsigned int *)(object + 0x454) = 0x11223344;
    *(unsigned int *)(object + 0x45c) = 0x55667788;
    *(unsigned int *)(object + 0x498) = 0x99aabbcc;
    *(unsigned int *)(object + 0x4a0) = 0xddeeff00;
    *(unsigned int *)(object + 0x418) = 0x3fc00000;
    object_state_save((char *)object);
    put(*(unsigned int *)(object + 0x160) == 0x11223344
        && *(unsigned int *)(object + 0x168) == 0x55667788
        && *(unsigned int *)(object + 0x16c) == 0x99aabbcc
        && *(unsigned int *)(object + 0x174) == 0xddeeff00 ? '0' : 'x');
    put(*(unsigned int *)(object + 0x178) == 0x3fc00000 ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
