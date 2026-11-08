/* The test harness for the rebuilt `player_route_camera` module.
 *
 * The harness holds the route table, the node array, the car object, the view
 * object, the three constants, and the list refresh stub.  The stub counts its
 * calls and saves the table, the name, and the two slot pointers.
 *
 * The harness runs eight paths:
 *
 *     0  one node at (1200, 1600, 3464): the view takes the node position,
 *        the yaw and the pitch, and the distance 2499 moves the view point
 *     1  one node with a negative z offset: the yaw reflects by 2 pi and the
 *        view point moves to the negative z side
 *     2  one node inside the 1500-unit limit: the view stays at the node
 *     3  one node at the exact 1500-unit limit: the view stays at the node
 *     4  a zero-distance node is skipped and the second node becomes the view
 *     5  two nonzero nodes: the nearer node becomes the view
 *     6  a car offset of 2000 on x: the deltas and the moved view follow the
 *        car position
 *     D  the exit marker
 *
 * The float constants and the expected integers come from the same arithmetic
 * as the original.  The final integers sit away from a truncation boundary, so
 * the hardware sine and cosine cannot change them.
 */
#pragma off (check_stack)

#define NODE_SIZE 0x38
#define NODE_COUNT 4

void player_route_camera(char *car, char *view);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static char car[0x500];
static char view[0x40];
static char nodes[NODE_COUNT * NODE_SIZE];
static char route[0x20];
static int refresh_calls;
static void *refresh_table;
static char *refresh_name;
static int *refresh_slot;
static int *refresh_slot_next;

char *DAT_0010f910;
float player_route_two_pi;
float player_route_minus_half_pi;
double player_route_minus_1500;

void object_list_refresh(void *table, char *name, int *slot)
{
    if (refresh_calls == 0) {
        refresh_table = table;
        refresh_name = name;
        refresh_slot = slot;
    } else if (refresh_calls == 1) {
        refresh_slot_next = slot;
    }
    refresh_calls = refresh_calls + 1;
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

static void set_bits_double(double *p, unsigned int low, unsigned int high)
{
    union {
        double d;
        unsigned int u[2];
    } value;

    value.u[0] = low;
    value.u[1] = high;
    *p = value.d;
}

static void reset_case(void)
{
    clear_bytes(car, 0x500);
    clear_bytes(view, 0x40);
    clear_bytes(nodes, NODE_COUNT * NODE_SIZE);
    clear_bytes(route, 0x20);

    *(char **)route = nodes;
    *(int *)(route + 0x1c) = 0;
    DAT_0010f910 = route;

    set_bits_float(&player_route_two_pi, 0x40c90fdb);
    set_bits_float(&player_route_minus_half_pi, 0xbfc90fdb);
    set_bits_double(&player_route_minus_1500, 0x00000000, 0xc0977000);

    refresh_calls = 0;
    refresh_table = 0;
    refresh_name = 0;
    refresh_slot = 0;
    refresh_slot_next = 0;
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

static int view_is(int x, int y, int z)
{
    return *(int *)(view + 0x00) == x
        && *(int *)(view + 0x04) == y
        && *(int *)(view + 0x08) == z;
}

static int view_prev_is(int x, int z)
{
    return *(int *)(view + 0x0c) == x
        && *(int *)(view + 0x14) == z;
}

static int refresh_is_ready(void)
{
    return refresh_calls == 2
        && refresh_table == (void *)0x6530
        && refresh_name == (char *)0x6350
        && refresh_slot == (int *)(car + 0x460)
        && refresh_slot_next == (int *)(car + 0x468);
}

int main(void)
{
    reset_case();
    set_count(1);
    set_node(0, 1200, 1600, 3464);
    player_route_camera(car, view);
    put(view_is(-531, 2214, 301) && view_prev_is(1200, 1600)
        && *(int *)(view + 0x38) == 0 && refresh_is_ready()
        ? '0' : 'x');

    reset_case();
    set_count(1);
    set_node(0, 1200, -1600, 3464);
    player_route_camera(car, view);
    put(view_is(-531, 2214, -301) && view_prev_is(1200, -1600)
        && *(int *)(view + 0x38) == 0 && refresh_is_ready()
        ? '1' : 'x');

    reset_case();
    set_count(1);
    set_node(0, 300, 400, 0);
    player_route_camera(car, view);
    put(view_is(300, 0, 400) && view_prev_is(300, 400)
        && *(int *)(view + 0x38) == 0 && refresh_is_ready()
        ? '2' : 'x');

    reset_case();
    set_count(1);
    set_node(0, 900, 1200, 0);
    player_route_camera(car, view);
    put(view_is(900, 0, 1200) && view_prev_is(900, 1200)
        && *(int *)(view + 0x38) == 0 && refresh_is_ready()
        ? '3' : 'x');

    reset_case();
    set_count(2);
    set_node(0, 0, 0, 0);
    set_node(1, 300, 400, 0);
    player_route_camera(car, view);
    put(view_is(300, 0, 400) && view_prev_is(300, 400)
        && *(int *)(view + 0x38) == 0 && refresh_is_ready()
        ? '4' : 'x');

    reset_case();
    set_count(2);
    set_node(0, 1200, 1600, 3464);
    set_node(1, 300, 400, 0);
    player_route_camera(car, view);
    put(view_is(300, 0, 400) && view_prev_is(300, 400)
        && *(int *)(view + 0x38) == 0 && refresh_is_ready()
        ? '5' : 'x');

    reset_case();
    set_count(1);
    set_node(0, 3200, 1600, 3464);
    set_car_int(0x4b0, 2000);
    player_route_camera(car, view);
    put(view_is(1468, 2214, 301) && view_prev_is(3200, 1600)
        && *(int *)(view + 0x38) == 0 && refresh_is_ready()
        ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
