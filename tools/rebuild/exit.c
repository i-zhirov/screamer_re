/* The rebuilt `exit` module.
 *
 * The original module holds two functions.  `exit` at linear 0x300be calls two
 * function-pointer slots and then the helper at 0x300d6; the helper calls the
 * second slot again, calls a third slot, and jumps to the process terminator at
 * 0x30387 (the dumps in docs/reference/program-map.md).
 *
 * The run-time source `bld/clib/startup/c/exit.c` of OpenWatcom V2 holds the same
 * shape for the DOS build: `exit` calls `(*__int23_exit)()` and `__FiniRtns`,
 * `_exit` calls `(*__int23_exit)()` and `(*__FPE_handler_exit)()`, and both end
 * in a call that never returns.  The shared slot is therefore `__int23_exit`.  The
 * names stay local to the rebuild, because the run-time symbol names of the original
 * build are unknown.
 *
 * The terminator carries `__declspec(__aborts)`, so the compiler drops the
 * epilogue of the helper and turns the last call into a jump, the same as the
 * original.
 */
extern void (*exit_hook_1)(void);
extern void (*exit_hook_2)(void);
extern void (*exit_hook_3)(void);
extern void exit_flush(int status);
extern __declspec(__aborts) void exit_terminate(int status);

void exit_flush(int status)
{
    (*exit_hook_2)();
    (*exit_hook_3)();
    exit_terminate(status);
}

void exit(int status)
{
    (*exit_hook_1)();
    (*exit_hook_2)();
    exit_flush(status);
}
