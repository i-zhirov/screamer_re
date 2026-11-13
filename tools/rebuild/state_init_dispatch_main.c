/* The test harness for the rebuilt `state_init_dispatch` module.
 *
 * The harness holds the state word and one stub for each screen initializer.
 * Each stub records its call count, so every path shows that the dispatch
 * reaches the right initializer.
 *
 * The harness runs eleven paths:
 *
 *     0  state 0: one `menu_frame` call
 *     1  state 1: no call
 *     2  state 2: one `car_select_screen` call
 *     3  state 3: one `track_select_screen` call
 *     4  state 4: one `name_entry_screen` call
 *     5  state 5: one `championship_screen_init` call
 *     6  state 6: one `race_records_screen_init` call
 *     7  state 7: no call
 *     8  state 8: one `net_race_lobby_init` call
 *     9  state 9: no call, because the unsigned compare rejects a value above 8
 *     D  the exit marker
 */
#pragma off (check_stack)

void state_init_dispatch(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_0011fac4;

static char text[8];
static int calls[7];

void menu_frame(void) { calls[0]++; }
void car_select_screen(void) { calls[1]++; }
void track_select_screen(void) { calls[2]++; }
void name_entry_screen(void) { calls[3]++; }
void championship_screen_init(void) { calls[4]++; }
void race_records_screen_init(void) { calls[5]++; }
void net_race_lobby_init(void) { calls[6]++; }

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(void)
{
    int index;

    for (index = 0; index < 7; index++) {
        calls[index] = 0;
    }
}

static int only(int index)
{
    int i;
    int ok;

    ok = calls[index] == 1;
    for (i = 0; i < 7; i++) {
        if (i != index && calls[i] != 0) {
            ok = 0;
        }
    }
    return ok;
}

static int none(void)
{
    int i;

    for (i = 0; i < 7; i++) {
        if (calls[i] != 0) {
            return 0;
        }
    }
    return 1;
}

static void run_state(unsigned int state, int index, char marker)
{
    int ok;

    reset();
    DAT_0011fac4 = state;
    state_init_dispatch();
    if (index < 0) {
        ok = none();
    } else {
        ok = only(index);
    }
    put(ok ? marker : 'x');
}

int main(void)
{
    run_state(0, 0, '0');
    run_state(1, -1, '1');
    run_state(2, 1, '2');
    run_state(3, 2, '3');
    run_state(4, 3, '4');
    run_state(5, 4, '5');
    run_state(6, 5, '6');
    run_state(7, -1, '7');
    run_state(8, 6, '8');
    run_state(9, -1, '9');
    put('D');
    quit();
    return 0;
}
