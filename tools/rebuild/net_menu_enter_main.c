/* The test harness for the rebuilt `net_menu_enter` module.
 *
 * The harness holds the resource set DAT_000062ac and the stubs of
 * net_player_add at 0x2dddc, net_session_init at 0xc6f80, resource_get at
 * 0x2eed0, memory_error_report at 0x2edad, and menu_enter at 0x28b5c.  Every
 * stub records its arguments and its call order.
 *
 * The paths:
 *
 *     0  a non-zero resource result calls the player add with 1, the session
 *        init, the resource get with NET_SPACE and 0x2000, and the menu enter,
 *        and skips the error report
 *     1  a zero resource result adds the error report before the menu enter
 *     2  a changed resource set passes the new pointer to the resource get
 *        and the error report
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void net_menu_enter(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_000062ac;

static char text[8];
static int sequence;
static int player_calls;
static int player_id;
static int player_order;
static int session_calls;
static int session_order;
static int resource_calls;
static void *resource_resource;
static int resource_size;
static int resource_name_ok;
static int resource_order;
static int error_calls;
static void *error_resource;
static int error_order;
static int menu_calls;
static int menu_order;
static void *resource_result;

static int names_equal(char *left, char *right)
{
    int index;

    index = 0;
    while (right[index] != 0) {
        if (left[index] != right[index]) {
            return 0;
        }
        index = index + 1;
    }
    return left[index] == 0;
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

static void check(int number, int ok)
{
    put(ok ? (char)('0' + number) : 'x');
}

static void reset(void)
{
    sequence = 0;
    player_calls = 0;
    player_id = 0;
    player_order = 0;
    session_calls = 0;
    session_order = 0;
    resource_calls = 0;
    resource_resource = 0;
    resource_size = 0;
    resource_name_ok = 0;
    resource_order = 0;
    error_calls = 0;
    error_resource = 0;
    error_order = 0;
    menu_calls = 0;
    menu_order = 0;
    resource_result = 0;
}

int net_player_add(int id)
{
    player_calls = player_calls + 1;
    player_id = id;
    sequence = sequence + 1;
    player_order = sequence;
    return 0;
}

int net_session_init(void)
{
    session_calls = session_calls + 1;
    sequence = sequence + 1;
    session_order = sequence;
    return 1;
}

void *resource_get(void *resource, char *name, int size)
{
    resource_calls = resource_calls + 1;
    resource_resource = resource;
    resource_size = size;
    resource_name_ok = names_equal(name, "NET_SPACE");
    sequence = sequence + 1;
    resource_order = sequence;
    return resource_result;
}

void memory_error_report(void *resource)
{
    error_calls = error_calls + 1;
    error_resource = resource;
    sequence = sequence + 1;
    error_order = sequence;
}

void menu_enter(void)
{
    menu_calls = menu_calls + 1;
    sequence = sequence + 1;
    menu_order = sequence;
}

int main(void)
{
    DAT_000062ac = (void *)0x1234;

    reset();
    resource_result = (void *)0x4000;
    net_menu_enter();
    check(0, player_calls == 1 && player_id == 1 && session_calls == 1
        && resource_calls == 1 && resource_resource == (void *)0x1234
        && resource_size == 0x2000 && resource_name_ok && error_calls == 0
        && menu_calls == 1 && player_order == 1 && session_order == 2
        && resource_order == 3 && menu_order == 4);

    DAT_000062ac = (void *)0x5678;
    reset();
    resource_result = 0;
    net_menu_enter();
    check(1, player_calls == 1 && player_id == 1 && session_calls == 1
        && resource_calls == 1 && resource_resource == (void *)0x5678
        && resource_size == 0x2000 && resource_name_ok && error_calls == 1
        && error_resource == (void *)0x5678 && menu_calls == 1
        && player_order == 1 && session_order == 2 && resource_order == 3
        && error_order == 4 && menu_order == 5);

    DAT_000062ac = (void *)0x9abc;
    reset();
    resource_result = (void *)0x8000;
    net_menu_enter();
    check(2, resource_calls == 1 && resource_resource == (void *)0x9abc
        && resource_name_ok && error_calls == 0 && menu_calls == 1
        && menu_order == sequence);

    put('D');
    quit();
    return 0;
}
