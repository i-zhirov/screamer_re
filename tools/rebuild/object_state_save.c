/* The rebuilt `object_state_save` module.
 *
 * The original sits at linear 0x1ccd8 in STARTH.EXE and holds 29 instructions
 * and 105 bytes: the two register saves, the frame, the object slot, the four
 * dword copies, the float copy at `+0x418` to `+0x178`, and the epilogue.
 *
 * The function saves the previous object position.  It copies the four dword
 * fields at `+0x454`, `+0x45c`, `+0x498`, and `+0x4a0` into `+0x160`, `+0x168`,
 * `+0x16c`, and `+0x174`, and the float at `+0x418` into `+0x178`.
 */
void object_state_save(char *object)
{
    *(unsigned int *)(object + 0x160) = *(unsigned int *)(object + 0x454);
    *(unsigned int *)(object + 0x168) = *(unsigned int *)(object + 0x45c);
    *(unsigned int *)(object + 0x16c) = *(unsigned int *)(object + 0x498);
    *(unsigned int *)(object + 0x174) = *(unsigned int *)(object + 0x4a0);
    *(float *)(object + 0x178) = *(float *)(object + 0x418);
}
