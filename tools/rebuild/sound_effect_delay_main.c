/* The test harness for the rebuilt `sound_effect_delay` module.
 *
 * The harness holds the frame delta and the per-effect timer array with one
 * extra sentinel entry.  The `sound_enqueue` stub counts the calls and records
 * the effect index, the variant mode, and the delay bits.
 *
 * The paths:
 *
 *     0  a clear enable signal resets the timer to 0 and enqueues nothing
 *     1  a latched timer below 0 returns and keeps the latch
 *     2  a timer below the limit adds the delta and enqueues nothing
 *     3  a timer that reaches the limit enqueues the effect and latches
 *     4  a timer that passes the limit enqueues the effect and latches
 *     5  the effect 0 adds the delta and leaves its neighbour unchanged
 *     6  a clear enable signal re-arms the effect for a new delay
 *     7  the limit 0 fires at the timer 0
 *     D  the exit marker
 *
 * The float values and the expected values come from their bit patterns, so
 * the harness does not depend on a float literal or on the constant segment.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

int sound_effect_delay(int enabled, int effect, float limit);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float DAT_00118e08;
float DAT_001922c[28];

static char text[8];
static int enqueue_calls;
static int enqueue_effect;
static int enqueue_mode;
static unsigned int enqueue_delay;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_f(float *dest, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *dest = value.f;
}

static unsigned int f_bits(float source)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.f = source;
    return value.u;
}

static float float_from_bits(unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    return value.f;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 28; index++) {
        set_f(&DAT_001922c[index], 0);
    }
    set_f(&DAT_00118e08, 0x3f800000);
    enqueue_calls = 0;
    enqueue_effect = 0;
    enqueue_mode = 0;
    enqueue_delay = 0;
}

void sound_enqueue(int effect, int mode, float delay)
{
    enqueue_calls = enqueue_calls + 1;
    enqueue_effect = effect;
    enqueue_mode = mode;
    enqueue_delay = f_bits(delay);
}

int main(void)
{
    int result;

    reset();
    set_f(&DAT_001922c[4], 0x40a00000);
    result = sound_effect_delay(0, 4, float_from_bits(0x40a00000));
    put(result == 0 && f_bits(DAT_001922c[4]) == 0x00000000
        && enqueue_calls == 0 ? '0' : 'x');

    reset();
    set_f(&DAT_001922c[1], 0xbf800000);
    set_f(&DAT_00118e08, 0x40000000);
    result = sound_effect_delay(1, 1, float_from_bits(0x41200000));
    put(result == 0 && f_bits(DAT_001922c[1]) == 0xbf800000
        && enqueue_calls == 0 ? '1' : 'x');

    reset();
    set_f(&DAT_001922c[2], 0x3f800000);
    set_f(&DAT_00118e08, 0x3f800000);
    result = sound_effect_delay(1, 2, float_from_bits(0x40a00000));
    put(result == 0 && f_bits(DAT_001922c[2]) == 0x40000000
        && enqueue_calls == 0 ? '2' : 'x');

    reset();
    set_f(&DAT_001922c[3], 0x40800000);
    set_f(&DAT_00118e08, 0x3f800000);
    result = sound_effect_delay(1, 3, float_from_bits(0x40a00000));
    put(result == 1 && f_bits(DAT_001922c[3]) == 0xbf800000
        && enqueue_calls == 1 && enqueue_effect == 3 && enqueue_mode == 3
        && enqueue_delay == 0x3f333333 ? '3' : 'x');

    reset();
    set_f(&DAT_001922c[5], 0x40900000);
    set_f(&DAT_00118e08, 0x3f800000);
    result = sound_effect_delay(1, 5, float_from_bits(0x40a00000));
    put(result == 1 && f_bits(DAT_001922c[5]) == 0xbf800000
        && enqueue_calls == 1 && enqueue_effect == 5 ? '4' : 'x');

    reset();
    set_f(&DAT_001922c[0], 0x40000000);
    set_f(&DAT_001922c[1], 0x40a00000);
    set_f(&DAT_00118e08, 0x3f800000);
    result = sound_effect_delay(1, 0, float_from_bits(0x40a00000));
    put(result == 0 && f_bits(DAT_001922c[0]) == 0x40400000
        && f_bits(DAT_001922c[1]) == 0x40a00000 && enqueue_calls == 0
        ? '5' : 'x');

    reset();
    set_f(&DAT_001922c[6], 0xbf800000);
    result = sound_effect_delay(0, 6, float_from_bits(0x40a00000));
    set_f(&DAT_00118e08, 0x40000000);
    result = sound_effect_delay(1, 6, float_from_bits(0x40a00000));
    put(result == 0 && f_bits(DAT_001922c[6]) == 0x40000000
        && enqueue_calls == 0 ? '6' : 'x');

    reset();
    set_f(&DAT_001922c[7], 0x00000000);
    set_f(&DAT_00118e08, 0x00000000);
    result = sound_effect_delay(1, 7, float_from_bits(0x00000000));
    put(result == 1 && f_bits(DAT_001922c[7]) == 0xbf800000
        && enqueue_calls == 1 && enqueue_effect == 7 ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
