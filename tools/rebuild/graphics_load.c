/* The rebuilt `graphics_load` module.
 *
 * The original sits at linear 0x10ba4 in STARTH.EXE and holds 40 instructions
 * and 88 bytes: the register saves, the frame, the two argument slots, the two
 * inline string copies, and the epilogue.  The two destinations are the
 * object-3 offsets 0x7f80 and 0x7fb4, so the loader relocates them to the two
 * static name buffers at linear 0x107f80 and 0x107fb4.
 *
 * The function copies the first name into the first buffer and the second name
 * into the second buffer.  The copy ends at the terminator.  The two buffers
 * are 0x34 bytes apart, so each holds a path of up to 0x33 characters and the
 * terminator.
 *
 * The `-oi` flag expands the `strcpy` call into the same two-byte copy loop as
 * the original.  The frame still differs: OpenWatcom V2 saves the registers
 * after the frame and keeps the arguments in registers, while the original
 * compiler saves the registers before the frame and spills the arguments to
 * the two slots.
 */
#include <string.h>

char graphics_name_a[0x34];
char graphics_name_b[0x34];

void graphics_load(char *name_a, char *name_b)
{
    strcpy(graphics_name_a, name_a);
    strcpy(graphics_name_b, name_b);
}
