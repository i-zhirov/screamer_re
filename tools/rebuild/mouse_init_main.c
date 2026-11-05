/* The test harness for the rebuilt `mouse_init` module.
 *
 * The harness holds the setup word, the two state areas, the three timer ids,
 * the far timer callback, and the library stubs.  The stubs record their
 * arguments and their call order.  The exit stub saves a snapshot at the moment
 * of the exit, because the module continues after a stub that returns.  The
 * harness runs five paths:
 *
 *     0  the first timer install fails and the module exits with -1
 *     1  the setup word is zero and the module returns after the first timer
 *     2  the success path installs the mouse handler and the 30 Hz timer
 *     3  the mouse handler install fails and the module cleans up and exits
 *     4  the 30 Hz timer install fails and the module cleans up and exits
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void mouse_init(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_00008058;
char DAT_00008060[0x20];
char DAT_00001e40[0x20];
unsigned int DAT_00001e50;
void (__far *DAT_00001e5c)(void);
unsigned int DAT_00018d8c;
unsigned int DAT_00018d90;
unsigned int DAT_00018d94;

static char text[8];

static int install_calls;
static int init_calls;
static int init_rate;
static int init_flags;
static int shutdown_calls;
static int shutdown_flags;
static int system_init_calls;
static int system_shutdown_calls;
static int handler_install_calls;
static int handler_remove_calls;
static int exit_calls;

static void __far *install_handler[4];
static void __far *install_id[4];
static int install_rate[4];
static int install_result[4];
static int handler_setup;
static void __far *handler_pointer;
static void __far *handler_state;
static void __far *handler_id;
static int handler_result;
static int remove_id;
static int remove_first;
static int remove_second;

struct exit_snapshot {
    int code;
    int install_calls;
    int shutdown_calls;
    int system_shutdown_calls;
    int handler_remove_calls;
};

static struct exit_snapshot snapshots[4];

void __far timer_tick_100(void) { }
void __far timer_tick_30(void) { }

int timer_init(int rate, int flags)
{
    init_calls = init_calls + 1;
    init_rate = rate;
    init_flags = flags;
    return 0;
}

int timer_install(int rate, void __far *handler, void __far *timer_id)
{
    int index;

    index = install_calls;
    if (index < 4) {
        install_rate[index] = rate;
        install_handler[index] = handler;
        install_id[index] = timer_id;
    }
    install_calls = install_calls + 1;
    return index < 4 ? install_result[index] : 0;
}

int timer_shutdown(int flags)
{
    shutdown_calls = shutdown_calls + 1;
    shutdown_flags = flags;
    return 0;
}

int mouse_system_init(int a, int b, int c)
{
    system_init_calls = system_init_calls + 1;
    return 0;
}

int mouse_system_shutdown(void)
{
    system_shutdown_calls = system_shutdown_calls + 1;
    return 0;
}

int mouse_handler_install(int setup, void __far *handler, void __far *state,
                          void __far *id)
{
    handler_install_calls = handler_install_calls + 1;
    handler_setup = setup;
    handler_pointer = handler;
    handler_state = state;
    handler_id = id;
    return handler_result;
}

int mouse_handler_remove(int handler, int first, int second)
{
    handler_remove_calls = handler_remove_calls + 1;
    remove_id = handler;
    remove_first = first;
    remove_second = second;
    return 0;
}

void exit(int code)
{
    int index;

    index = exit_calls;
    if (index < 4) {
        snapshots[index].code = code;
        snapshots[index].install_calls = install_calls;
        snapshots[index].shutdown_calls = shutdown_calls;
        snapshots[index].system_shutdown_calls = system_shutdown_calls;
        snapshots[index].handler_remove_calls = handler_remove_calls;
    }
    exit_calls = exit_calls + 1;
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

static void reset(void)
{
    int index;

    install_calls = 0;
    init_calls = 0;
    init_rate = 0;
    init_flags = -1;
    shutdown_calls = 0;
    shutdown_flags = -1;
    system_init_calls = 0;
    system_shutdown_calls = 0;
    handler_install_calls = 0;
    handler_remove_calls = 0;
    exit_calls = 0;
    handler_setup = 0;
    handler_pointer = 0;
    handler_state = 0;
    handler_id = 0;
    handler_result = 0;
    remove_id = 0;
    remove_first = 0;
    remove_second = 0;
    DAT_00001e50 = 0;
    DAT_00018d8c = 0;
    DAT_00018d90 = 0;
    DAT_00018d94 = 0x55;
    DAT_00001e5c = timer_tick_30;
    for (index = 0; index < 4; ++index) {
        install_rate[index] = 0;
        install_handler[index] = 0;
        install_id[index] = 0;
        install_result[index] = 0;
        snapshots[index].code = 0;
        snapshots[index].install_calls = 0;
        snapshots[index].shutdown_calls = 0;
        snapshots[index].system_shutdown_calls = 0;
        snapshots[index].handler_remove_calls = 0;
    }
}

static int first_timer_ok(void)
{
    return init_calls == 1 && init_rate == 0xff00 && init_flags == 0
        && install_calls >= 1 && install_rate[0] == 0x64
        && install_handler[0] == (void __far *)timer_tick_100
        && install_id[0] == (void __far *)&DAT_00018d8c;
}

int main(void)
{
    reset();
    DAT_00008058 = 0;
    install_result[0] = 1;
    mouse_init();
    put(first_timer_ok() && exit_calls == 1 && snapshots[0].code == -1
        && snapshots[0].install_calls == 1
        && snapshots[0].shutdown_calls == 0
        && snapshots[0].system_shutdown_calls == 0
        && handler_install_calls == 0 ? '0' : 'x');

    reset();
    DAT_00008058 = 0;
    install_result[0] = 0;
    mouse_init();
    put(first_timer_ok() && exit_calls == 0 && install_calls == 1
        && system_init_calls == 0 && handler_install_calls == 0
        && shutdown_calls == 0 ? '1' : 'x');

    reset();
    DAT_00008058 = 0x1234;
    install_result[0] = 0;
    install_result[1] = 0;
    handler_result = 0;
    mouse_init();
    put(first_timer_ok() && exit_calls == 0 && install_calls == 2
        && system_init_calls == 1
        && DAT_00001e50 == 0x2b11
        && handler_install_calls == 1
        && handler_setup == 0x1234
        && handler_pointer == (void __far *)DAT_00008060
        && handler_state == (void __far *)DAT_00001e40
        && handler_id == (void __far *)&DAT_00018d94
        && install_rate[1] == 0x1e
        && install_handler[1] == (void __far *)timer_tick_30
        && install_id[1] == (void __far *)&DAT_00018d90
        && shutdown_calls == 0 && handler_remove_calls == 0 ? '2' : 'x');

    reset();
    DAT_00008058 = 0x1234;
    install_result[0] = 0;
    install_result[1] = 1;
    handler_result = 1;
    mouse_init();
    put(first_timer_ok() && exit_calls >= 1 && snapshots[0].code == 1
        && snapshots[0].install_calls == 1
        && snapshots[0].shutdown_calls == 1 && shutdown_flags == 0
        && snapshots[0].system_shutdown_calls == 1
        && snapshots[0].handler_remove_calls == 0 ? '3' : 'x');

    reset();
    DAT_00008058 = 0x1234;
    install_result[0] = 0;
    install_result[1] = 1;
    handler_result = 0;
    mouse_init();
    put(first_timer_ok() && exit_calls >= 1 && snapshots[0].code == 1
        && snapshots[0].install_calls == 2
        && snapshots[0].shutdown_calls == 1 && shutdown_flags == 0
        && snapshots[0].handler_remove_calls == 1
        && remove_id == 0x55 && remove_first == 1 && remove_second == 1
        ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
