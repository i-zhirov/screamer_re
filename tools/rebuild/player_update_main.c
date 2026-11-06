/* The test harness for the rebuilt `player_update` module.
 *
 * The harness holds the player structure and the five step stubs.  Each stub
 * prints its marker, so the output shows the call order:
 *
 *     0  `player_step_0` ran
 *     1  `player_step_1` ran
 *     2  `player_step_2` ran
 *     3  `player_step_3` ran
 *     4  `player_step_4` ran
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void player_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_00110a20[0x200];

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

void player_step_0(void *player) { put('0'); }
void player_step_1(void *player) { put('1'); }
void player_step_2(void *player) { put('2'); }
void player_step_3(void *player) { put('3'); }
void player_step_4(void *player) { put('4'); }

int main(void)
{
    player_update();
    put('D');
    quit();
    return 0;
}
