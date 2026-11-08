/* The rebuilt `filter_update` module.
 *
 * The original sits at linear 0x22b80 in STARTH.EXE and holds 20 instructions
 * and 49 bytes: the `edx` save, the frame, the argument slot, the table pointer
 * loaded from `DAT_0010cf08`, the index scaled by 4, the one-pole filter
 * arithmetic, and the `ret 8` epilogue.
 *
 * The function applies the one-pole filter `*p = (*p - f) * table[i] + f`.
 * The first argument arrives in `eax`; the float argument makes the Watcom
 * compiler pass the remaining arguments on the stack, so the callee cleans
 * them with `ret 8`.
 */
extern float *DAT_0010cf08;

void filter_update(float *p, float f, int i)
{
    *p = (*p - f) * DAT_0010cf08[i] + f;
}
