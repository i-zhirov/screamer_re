/* The test harness for the rebuilt `race_track_load` module.
 *
 * The harness holds the two callee stubs.  Each stub records the call count
 * and the load value and returns a chosen result.
 *
 * The harness runs five paths:
 *
 *     0  both callees succeed: the function returns 0 and calls both once with
 *        the level value
 *     1  the route setup fails: the function returns the result and skips the
 *        records init
 *     2  the records init fails: the function returns the result after both
 *        calls
 *     3  both callees fail: the function returns the route setup result and
 *        skips the records init
 *     4  a large level value: both calls take the exact value
 *     D  the exit marker
 */
#pragma off (check_stack)

int race_track_load(int load);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static int route_calls;
static int route_value;
static int route_result;
static int records_calls;
static int records_value;
static int records_result;

int opponent_route_setup(int load)
{
    route_calls = route_calls + 1;
    route_value = load;
    return route_result;
}

int opponent_records_init(int load)
{
    records_calls = records_calls + 1;
    records_value = load;
    return records_result;
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

static void reset(void)
{
    route_calls = 0;
    route_value = -1;
    route_result = 0;
    records_calls = 0;
    records_value = -1;
    records_result = 0;
}

int main(void)
{
    int result;

    reset();
    route_result = 0;
    records_result = 0;
    result = race_track_load(5);
    put(result == 0
        && route_calls == 1 && route_value == 5
        && records_calls == 1 && records_value == 5
        ? '0' : 'x');

    reset();
    route_result = 7;
    records_result = 0;
    result = race_track_load(6);
    put(result == 7
        && route_calls == 1 && route_value == 6
        && records_calls == 0
        ? '1' : 'x');

    reset();
    route_result = 0;
    records_result = 9;
    result = race_track_load(7);
    put(result == 9
        && route_calls == 1 && route_value == 7
        && records_calls == 1 && records_value == 7
        ? '2' : 'x');

    reset();
    route_result = -1;
    records_result = -2;
    result = race_track_load(8);
    put(result == -1
        && route_calls == 1 && route_value == 8
        && records_calls == 0
        ? '3' : 'x');

    reset();
    route_result = 0;
    records_result = 0;
    result = race_track_load(0x1234);
    put(result == 0
        && route_calls == 1 && route_value == 0x1234
        && records_calls == 1 && records_value == 0x1234
        ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
