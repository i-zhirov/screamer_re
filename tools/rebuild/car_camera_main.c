/* The test harness for the rebuilt `car_camera` module.
 *
 * The harness holds the route table, the node array, the car record, the five
 * constants, and the two callee stubs.  The `player_view_setup` and
 * `view_render` stubs count their calls and save the view pointer.
 *
 * The harness runs nine paths:
 *
 *     0  three nodes: the nearest nonzero node becomes the view, the changed
 *        record runs the render path
 *     1  a zero-distance node is skipped and the flag comes from the height
 *        modulo 10
 *     2  a distance above 1500 moves the view down by 500
 *     3  a distance below 1500 keeps the view in place
 *     4  two calls with the same node take the unchanged path, and the first
 *        call runs the setup path on the flag change
 *     5  height modulo 10 of 2 with a short saved distance sets the flag
 *     6  height modulo 10 of 2 with a long saved distance clears the flag
 *     7  height modulo 10 of 4 with a saved distance at least the horizontal
 *        distance sets the flag
 *     8  height modulo 10 of 4 with a saved distance below the horizontal
 *        distance clears the flag
 *     D  the exit marker
 *
 * The float constants arrive from their bit patterns, because the minimal
 * image does not load the constant segment.
 */
#pragma off (check_stack)

#define CAR_SIZE 0x590
#define NODE_SIZE 0x38
#define NODE_COUNT 4

void car_camera(char *car);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char car[CAR_SIZE];
static char nodes[NODE_COUNT * NODE_SIZE];
static char route[0x20];
static int player_calls;
static int render_calls;
static char *player_arg;
static char *render_arg;

char *DAT_0010f90c;
float DAT_0010092c;
float DAT_00100930;
float DAT_00100934;
int DAT_00101e04;
float car_camera_zero;
float car_camera_far_limit;

void player_view_setup(char *view)
{
    player_calls = player_calls + 1;
    player_arg = view;
}

void view_render(char *view)
{
    render_calls = render_calls + 1;
    render_arg = view;
}

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void clear_bytes(char *p, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        p[index] = 0;
    }
}

static void set_bits_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *p = value.f;
}

static void reset_case(void)
{
    clear_bytes(car, CAR_SIZE);
    clear_bytes(nodes, NODE_COUNT * NODE_SIZE);
    clear_bytes(route, 0x20);

    *(char **)route = nodes;
    *(int *)(route + 0x1c) = 0;
    DAT_0010f90c = route;

    set_bits_float(&DAT_0010092c, 0x40c90fdb);
    set_bits_float(&DAT_00100930, 0xbfc90fdb);
    set_bits_float(&DAT_00100934, 0xc4bb8000);
    set_bits_float(&car_camera_zero, 0);
    set_bits_float(&car_camera_far_limit, 0x4effffff);

    DAT_00101e04 = 0;
    player_calls = 0;
    render_calls = 0;
    player_arg = 0;
    render_arg = 0;
}

static void set_count(int count)
{
    *(int *)(route + 0x1c) = count;
}

static void set_node(int index, int x, int z, int y)
{
    *(int *)(nodes + index * NODE_SIZE + 4) = x;
    *(int *)(nodes + index * NODE_SIZE + 8) = z;
    *(int *)(nodes + index * NODE_SIZE + 0xc) = y;
}

static void set_car_int(int offset, int value)
{
    *(int *)(car + offset) = value;
}

static int car_int(int offset)
{
    return *(int *)(car + offset);
}

static int view_is(int x, int y, int z)
{
    return car_int(0x4f8) == x && car_int(0x4fc) == y
        && car_int(0x500) == z;
}

int main(void)
{
    reset_case();
    set_count(3);
    set_node(0, 300, 400, 0);
    set_node(1, 0, 0, 0);
    set_node(2, 60, 80, 0);
    car_camera(car);
    put(car_int(0x554) == 2 && view_is(60, 0, 80)
        && car_int(0x504) == 60 && car_int(0x50c) == 80
        && car_int(0x558) == 2 && car_int(0x564) == 1
        && car_int(0x568) == 0 && car_int(0x56c) == 0
        && car_int(0x574) == 1 && DAT_00101e04 == 0
        && player_calls == 0 && render_calls == 1
        && render_arg == car + 0x4f8 ? '0' : 'x');

    reset_case();
    set_count(2);
    set_node(0, 0, 0, 0);
    set_node(1, 50, 0, 1);
    car_camera(car);
    put(car_int(0x554) == 1 && view_is(50, 1, 0)
        && car_int(0x564) == 1 && car_int(0x568) == 1
        && car_int(0x574) == 1 && player_calls == 0
        && render_calls == 1 ? '1' : 'x');

    reset_case();
    set_count(1);
    set_node(0, 0, -2000, 3);
    set_car_int(0x49c, 3);
    set_car_int(0x558, 1);
    car_camera(car);
    put(view_is(0, -496, -2000) && car_int(0x568) == 1
        && car_int(0x574) == 1 && player_calls == 0
        && render_calls == 1 ? '2' : 'x');

    reset_case();
    set_count(1);
    set_node(0, 0, -1000, 3);
    set_car_int(0x49c, 3);
    set_car_int(0x558, 1);
    car_camera(car);
    put(view_is(0, 3, -1000) && car_int(0x568) == 1
        && car_int(0x574) == 1 && player_calls == 0
        && render_calls == 1 ? '3' : 'x');

    reset_case();
    set_count(1);
    set_node(0, 0, -1000, 1);
    car_camera(car);
    car_camera(car);
    put(car_int(0x574) == 1 && car_int(0x56c) == 0
        && car_int(0x564) == 0 && car_int(0x578) == 0
        && car_int(0x568) == 1 && player_calls == 1
        && render_calls == 2 && (int)*(double *)(car + 0x55c) == 1000
        && player_arg == car + 0x4f8 && render_arg == car + 0x4f8
        ? '4' : 'x');

    reset_case();
    set_count(1);
    set_node(0, 0, -1000, 2);
    set_car_int(0x558, 1);
    set_car_int(0x16c, 0);
    set_car_int(0x174, -1000);
    car_camera(car);
    put(car_int(0x568) == 1 && car_int(0x574) == 1
        && player_calls == 0 && render_calls == 1 ? '5' : 'x');

    reset_case();
    set_count(1);
    set_node(0, 0, -1000, 2);
    set_car_int(0x558, 1);
    set_car_int(0x16c, 0);
    set_car_int(0x174, 1000);
    car_camera(car);
    put(car_int(0x568) == 0 && car_int(0x574) == 1
        && player_calls == 0 && render_calls == 1 ? '6' : 'x');

    reset_case();
    set_count(1);
    set_node(0, 0, 0, 4);
    set_car_int(0x4a0, 1);
    set_car_int(0x558, 1);
    set_car_int(0x16c, 0);
    set_car_int(0x174, 1000);
    car_camera(car);
    put(car_int(0x568) == 1 && car_int(0x574) == 1
        && player_calls == 0 && render_calls == 1 ? '7' : 'x');

    reset_case();
    set_count(1);
    set_node(0, 0, 0, 4);
    set_car_int(0x4a0, -1000);
    set_car_int(0x558, 1);
    set_car_int(0x16c, 0);
    set_car_int(0x174, 0);
    car_camera(car);
    put(car_int(0x568) == 0 && car_int(0x574) == 1
        && player_calls == 0 && render_calls == 1 ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
