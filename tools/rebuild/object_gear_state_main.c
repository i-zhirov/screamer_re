/* The test harness for the rebuilt `object_gear_state` module.
 *
 * The harness holds an object record, sets the gear state at `+0x3f4`, calls
 * the module, and checks the gear field at `+0x4e8`.  The states 0, 1, and 2
 * map to the three field values, and the state 3 changes nothing.
 *
 *     0  the state 0 cleared the field
 *     1  the state 1 set the field to 0x8000
 *     2  the state 2 set the field to 0xff00
 *     D  the state 3 changed nothing, and the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void object_gear_state(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x500];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void run_state(unsigned int state, unsigned int expected)
{
    *(unsigned int *)(object + 0x3f4) = state;
    *(unsigned int *)(object + 0x4e8) = 0x12345678;
    object_gear_state(object);
    put(*(unsigned int *)(object + 0x4e8) == expected ? '1' : 'x');
}

int main(void)
{
    run_state(0, 0);
    run_state(1, 0x8000);
    run_state(2, 0xff00);
    run_state(3, 0x12345678);
    put('D');
    quit();
    return 0;
}
