/* The test harness for the rebuilt `player_nearby_mark` module.
 *
 * The harness holds four 0x590-byte car records at `DAT_00110ab8`, the record
 * count `DAT_0010f8ec`, and one 0x590-byte player object.  The player object
 * holds the nearby mask at `+0x104` and the two position fields at `+0x4b0`
 * and `+0x4b8`.  Each car record holds the physics gate at `+0x414` and the two
 * position fields at `+0x498` and `+0x4a0`.
 *
 * The harness runs these paths:
 *
 *     0  no record: a preset mask is cleared and the scan does not run
 *     1  a near record with a clear physics gate is skipped
 *     2  a record 2000 units out on x is skipped
 *     3  a record 2000 units out on z is skipped
 *     4  a record 100 units out on both axes sets its index bit
 *     5  the scan starts at index 0: records 0 and 2 set two index bits
 *     6  a record exactly 1500 units out on x is skipped
 *     7  a negative x delta of 1499 units sets the index bit
 *     8  a clear gate at index 0 and a near record at index 1 set bit 1
 *     9  a near x at index 0 with z 1500 and a near record at index 1 set bit 1
 *     D  the exit marker
 */
#pragma off (check_stack)

void player_nearby_mark(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STRIDE 0x590
#define CARS 4

unsigned char DAT_00110ab8[STRIDE * CARS];
int DAT_0010f8ec;
unsigned char player_obj[STRIDE];

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

static char *car(int index)
{
    return (char *)DAT_00110ab8 + index * STRIDE;
}

static char *self(void)
{
    return (char *)player_obj;
}

static void set_int(char *base, int offset, int value)
{
    *(int *)(base + offset) = value;
}

static int get_int(char *base, int offset)
{
    return *(int *)(base + offset);
}

static void clear_cars(void)
{
    int index;
    int offset;

    for (index = 0; index < CARS; index++) {
        for (offset = 0; offset < STRIDE; offset++) {
            car(index)[offset] = 0;
        }
    }
}

static void reset(void)
{
    clear_cars();
    DAT_0010f8ec = CARS;
    set_int(self(), 0x104, 0xffffffff);
    set_int(self(), 0x4b0, 0);
    set_int(self(), 0x4b8, 0);
}

int main(void)
{
    reset();
    DAT_0010f8ec = 0;
    player_nearby_mark(self());
    put(get_int(self(), 0x104) == 0 ? '0' : 'x');

    reset();
    DAT_0010f8ec = 1;
    set_int(car(0), 0x414, 0);
    set_int(car(0), 0x498, 100);
    set_int(car(0), 0x4a0, 100);
    player_nearby_mark(self());
    put(get_int(self(), 0x104) == 0 ? '1' : 'x');

    reset();
    DAT_0010f8ec = 1;
    set_int(car(0), 0x414, 1);
    set_int(car(0), 0x498, 2000);
    player_nearby_mark(self());
    put(get_int(self(), 0x104) == 0 ? '2' : 'x');

    reset();
    DAT_0010f8ec = 1;
    set_int(car(0), 0x414, 1);
    set_int(car(0), 0x4a0, 2000);
    player_nearby_mark(self());
    put(get_int(self(), 0x104) == 0 ? '3' : 'x');

    reset();
    DAT_0010f8ec = 1;
    set_int(car(0), 0x414, 1);
    set_int(car(0), 0x498, 100);
    set_int(car(0), 0x4a0, 100);
    player_nearby_mark(self());
    put(get_int(self(), 0x104) == 1 ? '4' : 'x');

    reset();
    DAT_0010f8ec = 3;
    set_int(car(0), 0x414, 1);
    set_int(car(0), 0x498, 100);
    set_int(car(0), 0x4a0, 100);
    set_int(car(1), 0x414, 0);
    set_int(car(2), 0x414, 1);
    set_int(car(2), 0x498, 100);
    set_int(car(2), 0x4a0, 100);
    player_nearby_mark(self());
    put(get_int(self(), 0x104) == 5 ? '5' : 'x');

    reset();
    DAT_0010f8ec = 1;
    set_int(car(0), 0x414, 1);
    set_int(car(0), 0x498, 1500);
    player_nearby_mark(self());
    put(get_int(self(), 0x104) == 0 ? '6' : 'x');

    reset();
    DAT_0010f8ec = 1;
    set_int(car(0), 0x414, 1);
    set_int(car(0), 0x498, -1499);
    player_nearby_mark(self());
    put(get_int(self(), 0x104) == 1 ? '7' : 'x');

    reset();
    DAT_0010f8ec = 2;
    set_int(car(0), 0x414, 0);
    set_int(car(0), 0x498, 100);
    set_int(car(0), 0x4a0, 100);
    set_int(car(1), 0x414, 1);
    set_int(car(1), 0x498, 100);
    set_int(car(1), 0x4a0, 100);
    player_nearby_mark(self());
    put(get_int(self(), 0x104) == 2 ? '8' : 'x');

    reset();
    DAT_0010f8ec = 2;
    set_int(car(0), 0x414, 1);
    set_int(car(0), 0x498, 100);
    set_int(car(0), 0x4a0, 1500);
    set_int(car(1), 0x414, 1);
    set_int(car(1), 0x498, 100);
    set_int(car(1), 0x4a0, 100);
    player_nearby_mark(self());
    put(get_int(self(), 0x104) == 2 ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
