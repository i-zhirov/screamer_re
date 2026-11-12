/* The test harness for the rebuilt `player_previous_speed_set` module.
 *
 * The harness holds one player object.  The road speed sits at `+0x50c` and
 * the previous speed sits at `+0x588`, the same offsets as the original.  Two
 * sentinel fields at `+0x508` and `+0x58c` show that the function writes only
 * the previous speed.
 *
 * The paths:
 *
 *     0  the speed 1.0 overwrites the previous zero
 *     1  the negative speed -2.5 copies bit for bit
 *     2  the speed zero overwrites a set previous speed
 *     3  the small bit pattern 0x00000001 copies bit for bit
 *     4  the speed 1234.5 copies and keeps the road speed
 *     5  the NaN pattern 0x7fc00000 copies bit for bit
 *     6  the infinity pattern 0x7f800000 copies bit for bit
 *     D  the exit marker
 *
 * Every path checks the previous speed, the road speed, and both sentinels.
 */
#pragma off (check_stack)

void player_previous_speed_set(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define OBJECT_SIZE 0x600

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

static void set_bits(int offset, unsigned int bits)
{
    *(unsigned int *)(object + offset) = bits;
}

static unsigned int get_bits(int offset)
{
    return *(unsigned int *)(object + offset);
}

static void check(char marker, unsigned int speed, unsigned int before,
    unsigned int want)
{
    int good;

    set_bits(0x50c, speed);
    set_bits(0x588, before);
    set_bits(0x508, 0x5a5a5a5a);
    set_bits(0x58c, 0xa5a5a5a5);

    player_previous_speed_set(object);

    good = get_bits(0x588) == want
        && get_bits(0x50c) == speed
        && get_bits(0x508) == 0x5a5a5a5a
        && get_bits(0x58c) == 0xa5a5a5a5;

    put(good ? marker : 'x');
}

int main(void)
{
    check('0', 0x3f800000, 0x00000000, 0x3f800000);
    check('1', 0xc0200000, 0x00000000, 0xc0200000);
    check('2', 0x00000000, 0x40000000, 0x00000000);
    check('3', 0x00000001, 0x00000000, 0x00000001);
    check('4', 0x449a5000, 0x3f000000, 0x449a5000);
    check('5', 0x7fc00000, 0x00000000, 0x7fc00000);
    check('6', 0x7f800000, 0x00000000, 0x7f800000);

    put('D');
    quit();
    return 0;
}
