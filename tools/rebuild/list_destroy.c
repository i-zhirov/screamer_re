/* The rebuilt `list_destroy` module.
 *
 * The original sits at linear 0x2ed77 in STARTH.EXE and holds 25 instructions and 54
 * bytes: the register saves `push ebx`, `push ecx`, `push esi`, `push edi`, and
 * `push ebp`, the frame `mov ebp,esp` and `sub esp,0xc`, the two arguments stored at
 * `[ebp-8]` and `[ebp-4]`, the guard `cmp dword ptr [eax],0` with the skip to
 * `0x2ed9c`, the `free` call at `0xd2008` with the pointer argument, the return of the
 * first argument through the temporary at `[ebp-0xc]`, and the epilogue.
 *
 * The second argument reaches the frame but the body never reads it, so the source keeps it
 * as a parameter that the function ignores.  The guard frees the block only when the
 * pointer is set, and the function returns the first argument and leaves the pointer itself
 * unchanged, which the harness confirms.
 */
extern void free(void *block);

void *list_destroy(void **list, int unused)
{
    if (*list) {
        free(*list);
    }
    return list;
}
