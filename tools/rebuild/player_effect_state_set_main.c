/* The test harness for the rebuilt `player_effect_state_set` module.
 *
 * The harness holds one player object buffer with the effect state at `+0x148`,
 * the same offset as the original.
 *
 * The paths:
 *
 *     0  the state 0 and the value 2: the state becomes 2
 *     1  the state 0 and the value 5: the state becomes 5
 *     2  the state 2 and the value 5: the state stays 2
 *     3  the state 5 and the value 2: the state stays 5
 *     4  the state -1 and the value 2: the state stays -1
 *     5  the state 0 and the value 0: the state stays 0
 *     6  the state 0 and the value -1: the state becomes -1
 *     D  the exit marker
 *
 * Every path checks the state after the call.
 */
#pragma off (check_stack)

void player_effect_state_set(void *object, int state);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define OBJECT_SIZE 0x14c

static char text[8];
static unsigned char object[OBJECT_SIZE];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_state(int state)
{
    *(int *)(object + 0x148) = state;
}

static int get_state(void)
{
    return *(int *)(object + 0x148);
}

static void check(char number, int start, int value, int want)
{
    set_state(start);
    player_effect_state_set(object, value);
    put(get_state() == want ? number : 'x');
}

int main(void)
{
    check('0', 0, 2, 2);
    check('1', 0, 5, 5);
    check('2', 2, 5, 2);
    check('3', 5, 2, 5);
    check('4', -1, 2, -1);
    check('5', 0, 0, 0);
    check('6', 0, -1, -1);

    put('D');
    quit();
    return 0;
}
