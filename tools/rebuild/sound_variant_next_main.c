/* The test harness for the rebuilt `sound_variant_next` module.
 *
 * The harness holds the variant table, the quality flag, and the rotation
 * counters.  The table has 27 effects and three sample ids per effect, like the
 * table at object 3 offset 0x1fb4.  The value -1 marks a missing sample id.
 * The counters start at 2, so the first call returns the first sample id.
 *
 * The paths:
 *
 *     0  the zero quality flag returns the first sample id and leaves the
 *        counter unchanged
 *     1  the counter walks 10, 20, 30, and 10, so the wrap 2 to 0 works
 *     2  the -1 entry of 40, -1, 60 is skipped and the walk holds 40, 60, 40
 *     3  an all -1 effect returns -1 on every call
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#define EFFECT_COUNT 27

int sound_variant_next(int effect);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int sound_variant_table[EFFECT_COUNT][3];
int sound_quality_enabled;
int sound_rotation[EFFECT_COUNT];

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

static void reset(void)
{
    int index;

    for (index = 0; index < EFFECT_COUNT; ++index) {
        sound_variant_table[index][0] = 0;
        sound_variant_table[index][1] = 0;
        sound_variant_table[index][2] = 0;
        sound_rotation[index] = 2;
    }
    sound_variant_table[0][0] = 10;
    sound_variant_table[0][1] = 20;
    sound_variant_table[0][2] = 30;
    sound_variant_table[1][0] = 40;
    sound_variant_table[1][1] = -1;
    sound_variant_table[1][2] = 60;
    sound_variant_table[3][0] = -1;
    sound_variant_table[3][1] = -1;
    sound_variant_table[3][2] = -1;
    sound_quality_enabled = 0;
}

int main(void)
{
    int first;
    int second;
    int third;
    int fourth;

    reset();
    first = sound_variant_next(0);
    put(first == 10 && sound_rotation[0] == 2 ? '0' : 'x');

    reset();
    sound_quality_enabled = 1;
    first = sound_variant_next(0);
    second = sound_variant_next(0);
    third = sound_variant_next(0);
    fourth = sound_variant_next(0);
    put(first == 10 && second == 20 && third == 30 && fourth == 10
        && sound_rotation[0] == 0 ? '1' : 'x');

    reset();
    sound_quality_enabled = 1;
    first = sound_variant_next(1);
    second = sound_variant_next(1);
    third = sound_variant_next(1);
    put(first == 40 && second == 60 && third == 40 ? '2' : 'x');

    reset();
    sound_quality_enabled = 1;
    first = sound_variant_next(3);
    second = sound_variant_next(3);
    third = sound_variant_next(3);
    put(first == -1 && second == -1 && third == -1 ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
