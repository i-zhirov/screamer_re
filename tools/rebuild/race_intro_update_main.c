#pragma off (check_stack)

#include <stdarg.h>

void race_intro_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_00118e1c;
int DAT_00118e20;
float DAT_00118e24;
float DAT_00118e08;
int DAT_0010f8ec;
int DAT_0010634c;
unsigned char DAT_00110ab8[3 * 0x590];
int DAT_0000056c;
int DAT_00000570;
int DAT_00000574;
int DAT_00000578;
int DAT_0011cb18;
int DAT_0011cb1c;
int DAT_0011cb20;

double race_intro_phase = 0.5;
double race_intro_amplitude = 15000.0;

double sin(double value);

static char text[8];

static int effect_calls;
static int effect_value;
static int effect_arg;
static int panel_calls;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static unsigned int fbits(float value)
{
    return *(unsigned int *)&value;
}

static float wave(int index, float timer)
{
    return (float)(sin((double)index * race_intro_phase * (double)timer)
        * race_intro_amplitude);
}

static void set_int(int index, int offset, int value)
{
    *(int *)(DAT_00110ab8 + index * 0x590 + offset) = value;
}

static int get_int(int index, int offset)
{
    return *(int *)(DAT_00110ab8 + index * 0x590 + offset);
}

static void set_float(int index, int offset, float value)
{
    *(float *)(DAT_00110ab8 + index * 0x590 + offset) = value;
}

static float get_float(int index, int offset)
{
    return *(float *)(DAT_00110ab8 + index * 0x590 + offset);
}

static void reset(void)
{
    int index;

    DAT_00118e1c = 0;
    DAT_00118e20 = 0;
    DAT_00118e24 = 0.0f;
    DAT_00118e08 = 0.0f;
    DAT_0010f8ec = 0;
    DAT_0010634c = 0;
    for (index = 0; index < 3 * 0x590; index++) {
        DAT_00110ab8[index] = 0;
    }
    DAT_0000056c = 0;
    DAT_00000570 = 0;
    DAT_00000574 = 0;
    DAT_00000578 = 0;
    DAT_0011cb18 = 0;
    DAT_0011cb1c = 0;
    DAT_0011cb20 = 0;
    effect_calls = 0;
    effect_value = 0;
    effect_arg = 0;
    panel_calls = 0;
}

void effect_push(int value, int subtype)
{
    effect_calls = effect_calls + 1;
    effect_value = value;
    effect_arg = subtype;
}

void panel_update(void)
{
    panel_calls = panel_calls + 1;
}

int main(void)
{
    reset();
    DAT_00118e24 = 4.0f;
    DAT_00118e08 = 1.0f;
    DAT_0010f8ec = 1;
    set_float(0, 0x4f4, 3.0f);
    race_intro_update();
    put(fbits(DAT_00118e24) == 0x40800000u
            && effect_calls == 0
            && panel_calls == 0
            && fbits(get_float(0, 0x4f4)) == 0x40400000u
        ? '0' : 'x');

    reset();
    DAT_00118e1c = 2;
    DAT_00118e24 = 0.0f;
    DAT_00118e08 = 0.5f;
    DAT_0010f8ec = 2;
    set_float(0, 0x4f4, 1.0f);
    set_float(1, 0x4f4, 1.0f);
    race_intro_update();
    put(fbits(DAT_00118e24) == 0x3f000000u
            && DAT_00118e20 == 0
            && effect_calls == 1
            && effect_value == 0
            && effect_arg == 0
            && panel_calls == 1
            && DAT_0000056c == 0
            && DAT_00000570 == 0x280
            && DAT_00000574 == 0
            && DAT_00000578 == 0x190
            && DAT_0011cb18 == 0xf0
            && DAT_0011cb1c == 0x6c
            && DAT_0011cb20 == 0
            && fbits(get_float(0, 0x4f4)) == fbits(wave(0, DAT_00118e24))
            && fbits(get_float(1, 0x4f4)) == fbits(wave(1, DAT_00118e24))
        ? '1' : 'x');

    reset();
    DAT_00118e1c = 2;
    DAT_00118e24 = 5.0f;
    DAT_00118e08 = 1.0f;
    DAT_0010f8ec = 2;
    set_float(0, 0x4f4, 7.0f);
    set_float(1, 0x4f4, 8.0f);
    race_intro_update();
    put(fbits(DAT_00118e24) == 0x40c00000u
            && DAT_00118e20 == 3
            && (DAT_00118e1c & 1) == 1
            && (DAT_00118e1c & 2) == 2
            && effect_calls == 1
            && effect_value == 3
            && panel_calls == 1
            && DAT_0011cb20 == 3
            && get_int(0, 0x4f4) == 0
            && get_int(1, 0x4f4) == 0
        ? '2' : 'x');

    reset();
    DAT_00118e1c = 3;
    DAT_00118e24 = 7.0f;
    DAT_00118e08 = 1.0f;
    DAT_0010f8ec = 1;
    set_float(0, 0x4f4, 9.0f);
    race_intro_update();
    put(fbits(DAT_00118e24) == 0x41000000u
            && DAT_00118e20 == 3
            && (DAT_00118e1c & 2) == 0
            && (DAT_00118e1c & 1) == 1
            && effect_calls == 0
            && panel_calls == 1
            && DAT_0011cb20 == 3
            && fbits(get_float(0, 0x4f4)) == 0x41100000u
        ? '3' : 'x');

    reset();
    DAT_00118e1c = 2;
    DAT_00118e24 = 1.9f;
    DAT_00118e08 = 0.2f;
    DAT_0010f8ec = 2;
    race_intro_update();
    put(DAT_00118e20 == 1
            && effect_calls == 1
            && effect_value == 1
            && panel_calls == 1
            && DAT_0011cb20 == 1
            && fbits(get_float(0, 0x4f4)) == fbits(wave(0, DAT_00118e24))
            && fbits(get_float(1, 0x4f4)) == fbits(wave(1, DAT_00118e24))
        ? '4' : 'x');

    reset();
    DAT_00118e1c = 2;
    DAT_00118e24 = 9.0f;
    DAT_00118e08 = 2.0f;
    DAT_0010f8ec = 1;
    race_intro_update();
    put(DAT_00118e20 == 5
            && effect_calls == 1
            && effect_value == 5
            && panel_calls == 1
            && DAT_0011cb20 == 5
            && fbits(get_float(0, 0x4f4)) == fbits(wave(0, DAT_00118e24))
        ? '5' : 'x');

    reset();
    DAT_00118e1c = 3;
    DAT_00118e24 = 0.0f;
    DAT_00118e08 = 0.5f;
    DAT_0010f8ec = 1;
    set_float(0, 0x4f4, 4.0f);
    race_intro_update();
    put(DAT_00118e20 == 0
            && effect_calls == 1
            && effect_value == 0
            && panel_calls == 1
            && fbits(get_float(0, 0x4f4)) == 0x40800000u
        ? '6' : 'x');

    reset();
    DAT_00118e1c = 2;
    DAT_00118e24 = 2.0f;
    DAT_00118e08 = 0.5f;
    DAT_0010f8ec = 0;
    race_intro_update();
    put(DAT_00118e20 == 1
            && effect_calls == 0
            && panel_calls == 1
            && DAT_0010634c == 0
            && DAT_0011cb20 == 1
        ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
