/* The test harness for the rebuilt `car_collision_pass` module.
 *
 * The harness holds five 0x590-byte car records at `DAT_00110ab8`, the record
 * count `DAT_0010f8ec`, the crash speed threshold, and the collision response
 * stub.  The response stub records the call count and the contact distance,
 * the contact threshold, and the two deltas of the record at the call.
 *
 * The harness runs these paths:
 *
 *     0  no candidate: the scan does not run and the threshold keeps its value
 *     1  the mask leaves the candidate out: the threshold is set and no
 *        response runs
 *     2  the candidate physics gate is clear: the threshold is set and no
 *        response runs
 *     3  the first body-point pair is close: the response runs once with a
 *        zero distance, and the last deltas stay at the last pair
 *     4  a squared distance of 8 truncates to 2 and stays below the contact
 *        distance 3, so the response runs
 *     5  a squared distance of 4 gives the distance 2 and the equal contact
 *        distance 2 does not run the response
 *     6  the scan starts at the record number plus one and skips the inactive
 *        candidate: two responses run and the threshold takes the last value
 *     7  a speed drop above 5000.0 sets the crash state to 2
 *     8  a set crash state stays unchanged
 *     9  a speed drop of exactly 5000.0 sets no state
 *     A  a positive position sum selects the crash state 3
 *     B  a negative position sum also selects the crash state 3
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because an FPU memory
 * operand of a literal reads the constant pool at a low address in this
 * toolchain.
 */
#pragma off (check_stack)

void car_collision_pass(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STRIDE 0x590
#define CARS 5

unsigned char DAT_00110ab8[STRIDE * CARS];
int DAT_0010f8ec;
float collision_speed;

static char text[8];
static int response_calls;
static int response_distance;
static int response_threshold;
static int response_dx;
static int response_dz;

void collision_response(char *self, char *other)
{
    (void)other;
    response_calls = response_calls + 1;
    response_distance = *(int *)(self + 0x484);
    response_threshold = *(int *)(self + 0x488);
    response_dx = *(int *)(self + 0x48c);
    response_dz = *(int *)(self + 0x494);
}

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

static void set_speed(int index, int offset, unsigned int bits)
{
    *(unsigned int *)(car(index) + offset) = bits;
}

static void set_point(int index, int point, int x, int z)
{
    char *p;

    p = car(index);
    *(int *)(p + 0x434 + point * 12) = x;
    *(int *)(p + 0x43c + point * 12) = z;
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
    *(unsigned int *)&collision_speed = 0x459c4000;
    response_calls = 0;
    response_distance = 0;
    response_threshold = 0;
    response_dx = 0;
    response_dz = 0;
}

int main(void)
{
    char *self;

    reset();
    DAT_0010f8ec = 1;
    set_int(0, 0x488, 0x1111);
    self = car(0);
    car_collision_pass(self);
    put(response_calls == 0 && get_int(0, 0x488) == 0x1111
        && get_int(0, 0x2f8) == 0 ? '0' : 'x');

    reset();
    DAT_0010f8ec = 2;
    set_int(0, 0x44c, 10);
    set_int(1, 0x44c, 20);
    set_int(1, 0x414, 1);
    self = car(0);
    car_collision_pass(self);
    put(response_calls == 0 && get_int(0, 0x488) == 30 ? '1' : 'x');

    reset();
    DAT_0010f8ec = 2;
    set_int(0, 0xc, 0x2);
    set_int(0, 0x44c, 10);
    set_int(1, 0x44c, 20);
    self = car(0);
    car_collision_pass(self);
    put(response_calls == 0 && get_int(0, 0x488) == 30 ? '2' : 'x');

    reset();
    DAT_0010f8ec = 2;
    set_int(0, 0xc, 0x2);
    set_int(0, 0x44c, 10);
    set_int(1, 0x44c, 10);
    set_int(1, 0x414, 1);
    set_point(0, 0, 0, 0);
    set_point(0, 1, 0, 100);
    set_point(1, 0, 0, 0);
    set_point(1, 1, 100, 0);
    self = car(0);
    car_collision_pass(self);
    put(response_calls == 1 && response_distance == 0
        && response_threshold == 20 && response_dx == 0 && response_dz == 0
        && get_int(0, 0x48c) == -100 && get_int(0, 0x494) == 100
        ? '3' : 'x');

    reset();
    DAT_0010f8ec = 2;
    set_int(0, 0xc, 0x2);
    set_int(0, 0x44c, 1);
    set_int(1, 0x44c, 2);
    set_int(1, 0x414, 1);
    set_point(0, 0, 0, 0);
    set_point(0, 1, 0, 100);
    set_point(1, 0, 2, 2);
    set_point(1, 1, 100, 0);
    self = car(0);
    car_collision_pass(self);
    put(response_calls == 1 && response_distance == 2
        && response_threshold == 3 && response_dx == -2 && response_dz == -2
        ? '4' : 'x');

    reset();
    DAT_0010f8ec = 2;
    set_int(0, 0xc, 0x2);
    set_int(0, 0x44c, 1);
    set_int(1, 0x44c, 1);
    set_int(1, 0x414, 1);
    set_point(0, 0, 0, 0);
    set_point(0, 1, 10, 10);
    set_point(1, 0, 2, 0);
    set_point(1, 1, 12, 10);
    self = car(0);
    car_collision_pass(self);
    put(response_calls == 0 && get_int(0, 0x484) == 2
        && get_int(0, 0x488) == 2 ? '5' : 'x');

    reset();
    DAT_0010f8ec = 3;
    set_int(0, 0xc, 0xf);
    set_int(0, 0x44c, 1);
    set_int(1, 0x44c, 1);
    set_int(1, 0x414, 1);
    set_int(2, 0x44c, 50);
    set_point(0, 0, 0, 0);
    set_point(0, 1, 10, 10);
    set_point(1, 0, 0, 0);
    set_point(1, 1, 10, 10);
    set_point(2, 0, 0, 0);
    set_point(2, 1, 10, 10);
    self = car(0);
    car_collision_pass(self);
    put(response_calls == 2 && get_int(0, 0x488) == 51 ? '6' : 'x');

    reset();
    DAT_0010f8ec = 1;
    set_speed(0, 0x178, 0x45bb8000);
    set_speed(0, 0x418, 0x43fa0000);
    self = car(0);
    car_collision_pass(self);
    put(get_int(0, 0x2f8) == 2 ? '7' : 'x');

    reset();
    DAT_0010f8ec = 1;
    set_speed(0, 0x178, 0x45bb8000);
    set_speed(0, 0x418, 0x43fa0000);
    set_int(0, 0x2f8, 1);
    self = car(0);
    car_collision_pass(self);
    put(get_int(0, 0x2f8) == 1 ? '8' : 'x');

    reset();
    DAT_0010f8ec = 1;
    set_speed(0, 0x178, 0x45abe000);
    set_speed(0, 0x418, 0x43fa0000);
    self = car(0);
    car_collision_pass(self);
    put(get_int(0, 0x2f8) == 0 ? '9' : 'x');

    reset();
    DAT_0010f8ec = 1;
    set_speed(0, 0x178, 0x45bb8000);
    set_speed(0, 0x418, 0x43fa0000);
    set_int(0, 0x498, 4);
    self = car(0);
    car_collision_pass(self);
    put(get_int(0, 0x2f8) == 3 ? 'A' : 'x');

    reset();
    DAT_0010f8ec = 1;
    set_speed(0, 0x178, 0x45bb8000);
    set_speed(0, 0x418, 0x43fa0000);
    set_int(0, 0x498, -4);
    self = car(0);
    car_collision_pass(self);
    put(get_int(0, 0x2f8) == 3 ? 'B' : 'x');

    put('D');
    quit();
    return 0;
}
