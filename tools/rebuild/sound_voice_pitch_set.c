/* The rebuilt `sound_voice_pitch_set` module.
 *
 * The original sits at linear 0x1f7e4 in STARTH.EXE and holds 25 instructions
 * and 79 bytes up to the `ret` at 0x1f832.  The next function starts at
 * 0x1f834, so the store record keeps the 80-byte extent and sets no separate
 * function size.  The identity check uses 25 instructions.
 *
 * The three direct callers are `object_collision` at 0x1e00c,
 * `engine_rpm_update` at 0x18db0, and `sound_emitter_update` at 0x21b9c.  Each
 * caller passes the voice index in eax and the pitch value in edx.  The
 * callers sit in three sections, so the record takes the section Program
 * functions.
 *
 * The setup word `DAT_00008058` gates the store.  When the word is zero, the
 * function returns and stores nothing.  A zero pitch becomes 1.  The function
 * passes the driver handle `DAT_00018d94`, the voice index, and the pitch to
 * the driver value store at 0xc6af6.
 *
 * A pitch above 0x40000 prints the message `PITCH\n` through 0x3009b.  The
 * message sits at object-3 offset 0xbfa.  The caller data, the driver field at
 * voice offset 0x44, and the message support the name `sound_voice_pitch_set`.
 *
 * The original has the EBP frame and two locals, so the module builds with
 * `-of+`.  The function holds no floating-point instruction, so the module
 * needs no FPU stub.  The module uses no assembly implementation.
 */
extern int DAT_00008058;
extern int DAT_00018d94;
extern int driver_voice_value_set(int handle, int voice, int value);
extern int printf(const char *format, ...);

void sound_voice_pitch_set(int voice, int value)
{
    if (DAT_00008058 != 0) {
        if (value == 0) {
            value = 1;
        }
        driver_voice_value_set(DAT_00018d94, voice, value);
        if (value > 0x40000) {
            printf("PITCH\n");
        }
    }
}
