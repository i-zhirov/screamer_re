/* The test harness for the rebuilt `traffic_nearby_mark` module.
 *
 * The harness holds nine 0x2a8-byte traffic records at `DAT_00113cd8` and one
 * player object.  The player object holds the nearby mask at `+0x104` and the
 * two position fields at `+0x4b0` and `+0x4b8`.  Each traffic record holds its
 * state at `+0`, where a negative value means inactive, and the two position
 * fields at `+0x3c` and `+0x44`.
 *
 * The harness runs these paths:
 *
 *     0  every record is inactive: the preset mask is cleared
 *     1  a record 2000 units out on x is skipped
 *     2  a record 2000 units out on z is skipped
 *     3  a record 100 units out on both axes sets its index bit
 *     4  records 0 and 2 set two index bits
 *     5  a record exactly 1500 units out on x is skipped
 *     6  a negative x delta of 1499 units sets the index bit
 *     7  record 7 sets bit 7, so the scan reaches the last slot
 *     8  record 8 is near but past the fixed count 8, so the mask stays 0
 *     D  the exit marker
 */
#pragma off (check_stack)

void traffic_nearby_mark(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STRIDE 0x2a8
#define RECORDS 9

unsigned char DAT_00113cd8[STRIDE * RECORDS];
unsigned char player_obj[0x600];

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

static char *record(int index)
{
    return (char *)DAT_00113cd8 + index * STRIDE;
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

static void clear_records(void)
{
    int index;
    int offset;

    for (index = 0; index < RECORDS; index++) {
        for (offset = 0; offset < STRIDE; offset++) {
            record(index)[offset] = 0;
        }
    }
}

static void reset(void)
{
    int index;

    clear_records();
    for (index = 0; index < RECORDS; index++) {
        set_int(record(index), 0, -1);
    }
    set_int(self(), 0x104, 0xffffffff);
    set_int(self(), 0x4b0, 0);
    set_int(self(), 0x4b8, 0);
}

static void place(int index, int x, int z)
{
    set_int(record(index), 0, 0);
    set_int(record(index), 0x3c, x);
    set_int(record(index), 0x44, z);
}

int main(void)
{
    reset();
    traffic_nearby_mark(self());
    put(get_int(self(), 0x104) == 0 ? '0' : 'x');

    reset();
    place(0, 2000, 100);
    traffic_nearby_mark(self());
    put(get_int(self(), 0x104) == 0 ? '1' : 'x');

    reset();
    place(0, 100, 2000);
    traffic_nearby_mark(self());
    put(get_int(self(), 0x104) == 0 ? '2' : 'x');

    reset();
    place(0, 100, 100);
    traffic_nearby_mark(self());
    put(get_int(self(), 0x104) == 1 ? '3' : 'x');

    reset();
    place(0, 100, 100);
    place(2, 100, 100);
    traffic_nearby_mark(self());
    put(get_int(self(), 0x104) == 5 ? '4' : 'x');

    reset();
    place(0, 1500, 0);
    traffic_nearby_mark(self());
    put(get_int(self(), 0x104) == 0 ? '5' : 'x');

    reset();
    place(0, -1499, 0);
    traffic_nearby_mark(self());
    put(get_int(self(), 0x104) == 1 ? '6' : 'x');

    reset();
    place(7, 100, 100);
    traffic_nearby_mark(self());
    put(get_int(self(), 0x104) == 0x80 ? '7' : 'x');

    reset();
    place(8, 100, 100);
    traffic_nearby_mark(self());
    put(get_int(self(), 0x104) == 0 ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
