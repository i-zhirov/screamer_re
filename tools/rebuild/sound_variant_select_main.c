/* The test harness for the rebuilt `sound_variant_select` module.
 *
 * The harness holds the variant table, the quality flag, the sample count, and
 * the rotation counters.  The table has 27 effects and three sample ids per
 * effect, like the table at object 3 offset 0x1fb4.  The sample count is 100,
 * so a sample id at or above 100 is out of range.  The stub `sound_variant_next`
 * copies the rotating variant of 0x24b48: it walks the three sample ids of the
 * effect and skips the -1 entries.
 *
 * The paths:
 *
 *     0  the three fixed variants 0, 1, and 2 below the count
 *     1  the high-quality flag skips the clamp
 *     2  a high variant falls back to the first sample id
 *     3  a high first sample id falls back to -1
 *     4  a variant below the count stays unchanged
 *     5  the rotating mode walks 40, 60, and 40 over the -1 entry
 *     6  an all -1 effect returns -1 in every mode
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#define EFFECT_COUNT 27
#define SAMPLE_LIMIT 100

struct sound_entry {
    int effect;
    int mode;
};

int sound_variant_select(struct sound_entry *entry);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int sound_variant_table[EFFECT_COUNT][3];

int sound_quality_enabled;
int sound_sample_count;

static int sound_rotation[EFFECT_COUNT];
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
    sound_variant_table[2][0] = 120;
    sound_variant_table[2][1] = 5;
    sound_variant_table[2][2] = 130;
    sound_variant_table[3][0] = -1;
    sound_variant_table[3][1] = -1;
    sound_variant_table[3][2] = -1;
    sound_variant_table[4][0] = 7;
    sound_variant_table[4][1] = 200;
    sound_variant_table[4][2] = 9;
    sound_quality_enabled = 0;
    sound_sample_count = SAMPLE_LIMIT;
}

int sound_variant_next(int effect)
{
    int slot;
    int attempt;

    if (sound_quality_enabled == 0) {
        return sound_variant_table[effect][0];
    }
    for (attempt = 0; attempt < 3; ++attempt) {
        slot = sound_rotation[effect] + 1;
        if (slot == 3) {
            slot = 0;
        }
        sound_rotation[effect] = slot;
        if (sound_variant_table[effect][slot] != -1) {
            return sound_variant_table[effect][slot];
        }
    }
    return sound_variant_table[effect][sound_rotation[effect]];
}

int main(void)
{
    struct sound_entry entry;
    int first;
    int second;
    int third;

    reset();
    entry.effect = 0;
    entry.mode = 0;
    first = sound_variant_select(&entry);
    entry.mode = 1;
    second = sound_variant_select(&entry);
    entry.mode = 2;
    third = sound_variant_select(&entry);
    put(first == 10 && second == 20 && third == 30 ? '0' : 'x');

    reset();
    sound_quality_enabled = 1;
    entry.effect = 4;
    entry.mode = 1;
    put(sound_variant_select(&entry) == 200 ? '1' : 'x');

    reset();
    entry.effect = 4;
    entry.mode = 1;
    put(sound_variant_select(&entry) == 7 ? '2' : 'x');

    reset();
    entry.effect = 2;
    entry.mode = 0;
    put(sound_variant_select(&entry) == -1 ? '3' : 'x');

    reset();
    entry.effect = 1;
    entry.mode = 2;
    put(sound_variant_select(&entry) == 60 ? '4' : 'x');

    reset();
    sound_quality_enabled = 1;
    entry.effect = 1;
    entry.mode = 3;
    first = sound_variant_select(&entry);
    second = sound_variant_select(&entry);
    third = sound_variant_select(&entry);
    put(first == 40 && second == 60 && third == 40 ? '5' : 'x');

    reset();
    sound_quality_enabled = 1;
    entry.effect = 3;
    entry.mode = 0;
    first = sound_variant_select(&entry);
    entry.mode = 3;
    second = sound_variant_select(&entry);
    sound_quality_enabled = 0;
    entry.mode = 0;
    third = sound_variant_select(&entry);
    put(first == -1 && second == -1 && third == -1 ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
