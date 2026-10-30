/* The test harness for the rebuilt `object_player_input` module.
 *
 * The harness holds the object record, the input bits, and the three step
 * stubs.  Each stub counts its calls.  The harness runs the module with three
 * input states and checks the call set:
 *
 *     0  with the state clear and the bits set: all three steps ran
 *     1  with the state set and the bits set: the drive step did not run
 *     2  with the state clear and the bits clear: the drive step did not run
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void object_player_input(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_00118e1c;

static char text[8];
static unsigned char object[0x500];
static int proximity_calls;
static int drive_calls;
static int revolution_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void object_proximity_check(void *obj) { proximity_calls = proximity_calls + 1; }
void object_drive_step(void *obj) { drive_calls = drive_calls + 1; }
void object_revolution_step(void *obj) { revolution_calls = revolution_calls + 1; }

static void reset_counts(void)
{
    proximity_calls = 0;
    drive_calls = 0;
    revolution_calls = 0;
}

int main(void)
{
    *(unsigned int *)(object + 0x2f8) = 0;
    DAT_00118e1c = 5;
    reset_counts();
    object_player_input(object);
    put(proximity_calls == 1 && drive_calls == 1 && revolution_calls == 1
        ? '0' : 'x');
    *(unsigned int *)(object + 0x2f8) = 1;
    DAT_00118e1c = 5;
    reset_counts();
    object_player_input(object);
    put(proximity_calls == 1 && drive_calls == 0 && revolution_calls == 1
        ? '1' : 'x');
    *(unsigned int *)(object + 0x2f8) = 0;
    DAT_00118e1c = 0;
    reset_counts();
    object_player_input(object);
    put(proximity_calls == 1 && drive_calls == 0 && revolution_calls == 1
        ? '2' : 'x');
    put('D');
    quit();
    return 0;
}
