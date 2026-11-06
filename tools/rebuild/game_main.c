/* The rebuilt `main` module.
 *
 * The original sits at linear 0x10020 in STARTH.EXE and holds 56 instructions
 * and 194 bytes: the register saves `push ebx` and `push edx`, the frame `push
 * ebp` and `mov ebp,esp`, the local slot at `[ebp-4]`, the allocation of 0x4c
 * bytes, the list of 0x96 records, the memory check against 0x61a800, the two
 * messages of the low-memory path, the initialization, the race loop, the
 * error path, the shutdown, and the cleanup.
 *
 * The function allocates the list record, fills it with 0x96 records, and stores
 * the pointer in `DAT_000062ac`.  When the available memory is below 6400000 it
 * prints the two messages.  Otherwise it initializes the engine, runs the race
 * loop until the quit flag `DAT_00007ed8` is 1, and shuts the engine down.  A
 * non-zero race result goes through the video mode 3, the error report, and the
 * exit call.  The cleanup frees the list block and then the list record.
 *
 * The two format strings sit at the object 3 offsets 4 and 0x29.  The strings in
 * the image are `\nNot enough memory to play SCREAMER\n` and `You need another
 * %d bytes\n`.  The source passes the literals, so the compiler emits the long
 * `push imm32` with a relocation, as the `error_report` module does.
 *
 * The original keeps the list pointer in a register and spills the race result
 * to the one 4-byte local at `[ebp-4]`, then saves `ebx` for the result test.
 * The rebuild keeps both values in `edx` and holds no local.  That difference
 * keeps the identity check open.  The original has the EBP frame, so the module
 * builds with `-of+`.
 */
extern void *memory_alloc(unsigned int size);
extern void *list_create(void *list, int count);
extern unsigned int memory_available(void *pool);
extern int printf(const char *format, ...);
extern void engine_init(void);
extern int race_loop(void);
extern void bios_video(int mode);
extern void error_report(int code);
extern void exit_proc(int code);
extern void frame_update(void);
extern void engine_shutdown(void);
extern void *list_destroy(void *list, int unused);
extern void release(void *block);

extern void *DAT_000062ac;
extern unsigned int DAT_00007ed8;

void game_main(void)
{
    void *list;
    int result;

    list = memory_alloc(0x4c);
    if (list != 0) {
        list = list_create(list, 0x96);
    }
    DAT_000062ac = list;
    if (memory_available(list) < 0x61a800) {
        printf("\nNot enough memory to play SCREAMER\n");
        printf("You need another %d bytes\n",
            0x61a800 - memory_available(DAT_000062ac));
    } else {
        engine_init();
        while (DAT_00007ed8 != 1) {
            result = race_loop();
            if (result != 0) {
                bios_video(3);
                error_report(result);
                exit_proc(result);
            }
            frame_update();
        }
        engine_shutdown();
    }
    if (DAT_000062ac != 0) {
        release(list_destroy(DAT_000062ac, 0));
    }
}
