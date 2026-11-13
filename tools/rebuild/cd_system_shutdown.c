/* The rebuilt `cd_system_shutdown` module.
 *
 * The original sits at linear 0x25f14 in STARTH.EXE and holds 6 instructions
 * and 15 bytes up to the `ret` at 0x25f22.  One padding byte at 0x25f23
 * precedes the next function at 0x25f24, so the store record keeps the
 * 16-byte extent and sets no separate function size.  The identity check uses
 * 6 instructions.
 *
 * The only direct caller is the shutdown routine at 0x128ac with the call
 * site at 0x128d9.  The routine first releases the engine interface, the
 * video text mode, the keyboard hook, the music bank, the sound system, and
 * the DOS date and time, so the function runs at the end of the program
 * shutdown.
 *
 * The function calls `sound_driver_set` at 0x262a8 with 0 and then
 * `dpmi_free_dos` at 0x2666c.  The first helper clears the HMI driver state.
 * The second helper frees the DOS memory block at `DAT_001666a2`.  The
 * MSCDEX setup in `dpmi_real_int` at 0x25db0 allocates that block through the
 * DPMI function 0x0100 at 0x26640 and builds the CD request block on it, so the
 * function releases the CD subsystem.
 *
 * The caller, the two helpers, and the CD setup support the name
 * `cd_system_shutdown`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.  The caller and the helpers sit in the
 * shutdown path, so the module takes the section Subsystem shutdown.
 *
 * The original has no EBP frame and saves the registers with `pushal`, so the
 * module builds without `-of+`.  The function holds no floating-point
 * instruction, so it needs no FPU stub and no assembly body.
 */
extern void sound_driver_set(int value);
extern void dpmi_free_dos(void);

void cd_system_shutdown(void)
{
    sound_driver_set(0);
    dpmi_free_dos();
}
