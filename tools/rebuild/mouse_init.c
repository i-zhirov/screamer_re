/* The rebuilt `mouse_init` module.
 *
 * The original sits at linear 0x1f1a0 in STARTH.EXE and holds 67 instructions
 * and 233 bytes: the four register saves, the frame, the two timer rates, the
 * two timer installs at `0xc835b`, the mouse system calls at `0xc8f81` and
 * `0xc8ffa`, the handler install at `0xc903a`, the handler remove at
 * `0xc98f9`, the two exit paths at `0x300be`, and the epilogue.
 *
 * The function sets the timer rate, installs the 100 Hz timer at
 * `CS:0x20d` with its id at `DAT_00018d8c`, and returns when the setup word
 * `DAT_00008058` is zero.  When the setup word is set, it starts the mouse
 * system, installs the mouse handler with the state block at
 * `DAT_00001e40`, and installs the 30 Hz timer from the far pointer
 * `DAT_00001e5c`.  A failed install shuts the started parts down and exits.
 */
extern int timer_init(int rate, int flags);
extern int timer_install(int rate, void __far *handler, void __far *timer_id);
extern int timer_shutdown(int flags);
extern int mouse_system_init(int a, int b, int c);
extern int mouse_system_shutdown(void);
extern int mouse_handler_install(int setup, void __far *handler,
                                 void __far *state, void __far *handler_id);
extern int mouse_handler_remove(int handler_id, int a, int b);
extern void exit(int code);

extern unsigned int DAT_00008058;
extern char DAT_00008060[];
extern char DAT_00001e40[];
extern unsigned int DAT_00001e50;
extern void (__far *DAT_00001e5c)(void);
extern unsigned int DAT_00018d8c;
extern unsigned int DAT_00018d90;
extern unsigned int DAT_00018d94;

extern void __far timer_tick_100(void);

void mouse_init(void)
{
    int status;

    timer_init(0xff00, 0);
    if (timer_install(0x64, (void __far *)timer_tick_100,
                      (void __far *)&DAT_00018d8c) != 0) {
        exit(-1);
    }
    if (DAT_00008058 == 0) {
        return;
    }
    mouse_system_init(0, 0, 0);
    DAT_00001e50 = 0x2b11;
    status = mouse_handler_install(DAT_00008058, (void __far *)DAT_00008060,
        (void __far *)DAT_00001e40, (void __far *)&DAT_00018d94);
    if (status != 0) {
        timer_shutdown(0);
        mouse_system_shutdown();
        exit(1);
    }
    status = timer_install(0x1e, (void __far *)DAT_00001e5c,
                           (void __far *)&DAT_00018d90);
    if (status != 0) {
        timer_shutdown(0);
        mouse_handler_remove(DAT_00018d94, 1, 1);
        exit(1);
    }
}
