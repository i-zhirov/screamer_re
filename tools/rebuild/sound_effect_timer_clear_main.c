/* The test harness for the rebuilt `sound_effect_timer_clear` module.
 *
 * The harness holds the per-effect timer array with one extra sentinel entry.
 * Each case fills every cell with a sentinel, calls the function with one
 * index, and checks that only the target cell changes.
 *
 * The paths:
 *
 *     0  the first cell
 *     1  a middle cell
 *     2  the last effect cell, 26
 *     3  a cell that already holds 0
 *     4  two calls that clear two cells
 *     D  the exit marker
 *
 * The array holds integers, like the original, so the harness needs no float
 * symbol and no constant segment.
 */
#pragma off (check_stack)

#define EFFECT_COUNT 27
#define STORE_COUNT 28
#define SENTINEL 0x7e7e7e7e

void sound_effect_timer_clear(int effect);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_001922c[STORE_COUNT];

static char text[8];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void fill_cells(void)
{
    int index;

    for (index = 0; index < STORE_COUNT; index++) {
        DAT_001922c[index] = SENTINEL;
    }
}

static int only_cleared(int target)
{
    int index;

    for (index = 0; index < STORE_COUNT; index++) {
        if (index == target) {
            if (DAT_001922c[index] != 0) {
                return 0;
            }
        } else if (DAT_001922c[index] != SENTINEL) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int ok;

    fill_cells();
    sound_effect_timer_clear(0);
    put(only_cleared(0) ? '0' : 'x');

    fill_cells();
    sound_effect_timer_clear(15);
    put(only_cleared(15) ? '1' : 'x');

    fill_cells();
    sound_effect_timer_clear(26);
    put(only_cleared(26) ? '2' : 'x');

    fill_cells();
    DAT_001922c[7] = 0;
    sound_effect_timer_clear(7);
    put(only_cleared(7) ? '3' : 'x');

    fill_cells();
    sound_effect_timer_clear(3);
    sound_effect_timer_clear(4);
    ok = DAT_001922c[3] == 0
        && DAT_001922c[4] == 0
        && DAT_001922c[2] == SENTINEL
        && DAT_001922c[5] == SENTINEL;
    put(ok ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
