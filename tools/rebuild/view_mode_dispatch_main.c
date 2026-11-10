/* The test harness for the rebuilt `view_mode_dispatch` module.
 *
 * The harness holds a 0x100-byte view object and one stub for each camera
 * routine.  A stub records its call count and its arguments, so every path
 * shows that the dispatch reaches the right routine with the right arguments.
 *
 * The harness runs eleven paths:
 *
 *     0  mode 0: one cockpit call with the car 0xf91c and the view
 *     1  mode 1: one orbit call with the car 0xf91c, the view, and the mode 1
 *     2  mode 2: no call
 *     3  mode 3: one route call with the car 0xf91c and the view
 *     4  mode 4: no call
 *     5  mode 5: no call
 *     6  mode 6: one orbit call with the mode 2
 *     7  mode 7: no call
 *     8  mode 8: one select call with the view
 *     9  mode 9: one orbit call with the mode 0
 *     A  mode 10: no call, because the unsigned compare rejects a value above 9
 *     D  the exit marker
 */
#pragma off (check_stack)

struct player_view {
    int field_00[0x22];
    int mode;
};

void view_mode_dispatch(struct player_view *view);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static struct player_view view;

static int cockpit_calls;
static char *cockpit_car;
static char *cockpit_view;
static int orbit_calls;
static char *orbit_car;
static char *orbit_view;
static int orbit_mode;
static int route_calls;
static char *route_car;
static char *route_view;
static int select_calls;
static char *select_view;

void player_cockpit_camera(char *car, char *view)
{
    cockpit_calls++;
    cockpit_car = car;
    cockpit_view = view;
}

void player_view_orbit(char *car, char *view, int mode)
{
    orbit_calls++;
    orbit_car = car;
    orbit_view = view;
    orbit_mode = mode;
}

void player_route_camera(char *car, char *view)
{
    route_calls++;
    route_car = car;
    route_view = view;
}

void car_camera_select(char *view)
{
    select_calls++;
    select_view = view;
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

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x100; index++) {
        ((char *)&view)[index] = 0;
    }
    cockpit_calls = 0;
    cockpit_car = 0;
    cockpit_view = 0;
    orbit_calls = 0;
    orbit_car = 0;
    orbit_view = 0;
    orbit_mode = -1;
    route_calls = 0;
    route_car = 0;
    route_view = 0;
    select_calls = 0;
    select_view = 0;
}

static int no_calls(void)
{
    return cockpit_calls == 0 && orbit_calls == 0 && route_calls == 0
        && select_calls == 0;
}

int main(void)
{
    int ok;

    reset();
    view.mode = 0;
    view_mode_dispatch(&view);
    ok = cockpit_calls == 1 && orbit_calls == 0 && route_calls == 0
        && select_calls == 0 && cockpit_car == (char *)0xf91c
        && cockpit_view == (char *)&view;
    report(ok, '0');

    reset();
    view.mode = 1;
    view_mode_dispatch(&view);
    ok = no_calls() == 0 && orbit_calls == 1 && cockpit_calls == 0
        && route_calls == 0 && select_calls == 0
        && orbit_car == (char *)0xf91c && orbit_view == (char *)&view
        && orbit_mode == 1;
    report(ok, '1');

    reset();
    view.mode = 2;
    view_mode_dispatch(&view);
    report(no_calls(), '2');

    reset();
    view.mode = 3;
    view_mode_dispatch(&view);
    ok = cockpit_calls == 0 && orbit_calls == 0 && route_calls == 1
        && select_calls == 0 && route_car == (char *)0xf91c
        && route_view == (char *)&view;
    report(ok, '3');

    reset();
    view.mode = 4;
    view_mode_dispatch(&view);
    report(no_calls(), '4');

    reset();
    view.mode = 5;
    view_mode_dispatch(&view);
    report(no_calls(), '5');

    reset();
    view.mode = 6;
    view_mode_dispatch(&view);
    ok = cockpit_calls == 0 && orbit_calls == 1 && route_calls == 0
        && select_calls == 0 && orbit_car == (char *)0xf91c
        && orbit_view == (char *)&view && orbit_mode == 2;
    report(ok, '6');

    reset();
    view.mode = 7;
    view_mode_dispatch(&view);
    report(no_calls(), '7');

    reset();
    view.mode = 8;
    view_mode_dispatch(&view);
    ok = cockpit_calls == 0 && orbit_calls == 0 && route_calls == 0
        && select_calls == 1 && select_view == (char *)&view;
    report(ok, '8');

    reset();
    view.mode = 9;
    view_mode_dispatch(&view);
    ok = cockpit_calls == 0 && orbit_calls == 1 && route_calls == 0
        && select_calls == 0 && orbit_car == (char *)0xf91c
        && orbit_view == (char *)&view && orbit_mode == 0;
    report(ok, '9');

    reset();
    view.mode = 10;
    view_mode_dispatch(&view);
    report(no_calls(), 'A');

    put('D');
    quit();
    return 0;
}
