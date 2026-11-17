/* The test harness for the rebuilt `car_nearby_mark` module.
 *
 * The harness holds five 0x590-byte car records at `DAT_00110ab8` and the
 * record count `DAT_0010f8ec`.  Each record holds its own number at `+0x00`,
 * the nearby mask at `+0x0c`, the physics gate at `+0x414`, and the two
 * position fields at `+0x498` and `+0x4a0`.
 *
 * The harness runs these paths:
 *
 *     0  no later record: a preset mask is cleared and the scan does not run
 *     1  a near candidate with a clear physics gate is skipped
 *     2  a candidate 2000 units out on x is skipped
 *     3  a candidate 2000 units out on z is skipped
 *     4  a candidate 100 units out on both axes sets its index bit
 *     5  the scan starts after the own record: an earlier near record is not
 *        scanned and a later near record sets its own bit
 *     6  two near candidates set two index bits
 *     7  a candidate exactly 1500 units out on x is skipped
 *     8  a negative x delta of 1499 units sets the index bit
 *     9  a near candidate sets its bit and a candidate 1500 units out on z is
 *        skipped
 *     D  the exit marker
 */
#pragma off (check_stack)

void car_nearby_mark(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STRIDE 0x590
#define CARS 5

unsigned char DAT_00110ab8[STRIDE * CARS];
int DAT_0010f8ec;

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

static void set_int(int index, int offset, int value)
{
    *(int *)(car(index) + offset) = value;
}

static int get_int(int index, int offset)
{
    return *(int *)(car(index) + offset);
}

static void clear_cars(void)
{
    int index;
    int offset;

    for (index = 0; index < CARS; index++) {
        for (offset = 0; offset < STRIDE; offset++) {
            car(index)[offset] = 0;
        }
        set_int(index, 0, index);
    }
}

static void reset(void)
{
    clear_cars();
    DAT_0010f8ec = CARS;
}

int main(void)
{
    char *self;

    reset();
    DAT_0010f8ec = 1;
    set_int(0, 0xc, 0xffffffff);
    self = car(0);
    car_nearby_mark(self);
    put(get_int(0, 0xc) == 0 ? '0' : 'x');

    reset();
    DAT_0010f8ec = 2;
    set_int(1, 0x414, 0);
    set_int(1, 0x498, 100);
    set_int(1, 0x4a0, 100);
    self = car(0);
    car_nearby_mark(self);
    put(get_int(0, 0xc) == 0 ? '1' : 'x');

    reset();
    DAT_0010f8ec = 2;
    set_int(1, 0x414, 1);
    set_int(1, 0x498, 2000);
    self = car(0);
    car_nearby_mark(self);
    put(get_int(0, 0xc) == 0 ? '2' : 'x');

    reset();
    DAT_0010f8ec = 2;
    set_int(1, 0x414, 1);
    set_int(1, 0x4a0, 2000);
    self = car(0);
    car_nearby_mark(self);
    put(get_int(0, 0xc) == 0 ? '3' : 'x');

    reset();
    DAT_0010f8ec = 2;
    set_int(1, 0x414, 1);
    set_int(1, 0x498, 100);
    set_int(1, 0x4a0, 100);
    self = car(0);
    car_nearby_mark(self);
    put(get_int(0, 0xc) == 2 ? '4' : 'x');

    reset();
    DAT_0010f8ec = 3;
    set_int(0, 0x414, 1);
    set_int(0, 0x498, 100);
    set_int(0, 0x4a0, 100);
    set_int(2, 0x414, 1);
    set_int(2, 0x498, 100);
    set_int(2, 0x4a0, 100);
    self = car(1);
    car_nearby_mark(self);
    put(get_int(1, 0xc) == 4 ? '5' : 'x');

    reset();
    DAT_0010f8ec = 4;
    set_int(1, 0x414, 1);
    set_int(1, 0x498, 100);
    set_int(3, 0x414, 1);
    set_int(3, 0x4a0, 100);
    set_int(2, 0x414, 1);
    set_int(2, 0x498, 3000);
    self = car(0);
    car_nearby_mark(self);
    put(get_int(0, 0xc) == 0xa ? '6' : 'x');

    reset();
    DAT_0010f8ec = 2;
    set_int(1, 0x414, 1);
    set_int(1, 0x498, 1500);
    self = car(0);
    car_nearby_mark(self);
    put(get_int(0, 0xc) == 0 ? '7' : 'x');

    reset();
    DAT_0010f8ec = 2;
    set_int(1, 0x414, 1);
    set_int(1, 0x498, 1499);
    self = car(0);
    car_nearby_mark(self);
    put(get_int(0, 0xc) == 2 ? '8' : 'x');

    reset();
    DAT_0010f8ec = 3;
    set_int(1, 0x414, 1);
    set_int(1, 0x498, 100);
    set_int(2, 0x414, 1);
    set_int(2, 0x4a0, 1500);
    self = car(0);
    car_nearby_mark(self);
    put(get_int(0, 0xc) == 2 ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
