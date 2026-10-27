/* The rebuilt `free` module.
 *
 * The original module sits at linear 0x300ec in STARTH.EXE and holds three
 * instructions: `test eax,eax`, `jne 0xd2008`, `ret` (the dump in
 * docs/reference/program-map.md).  The Watcom register calling convention
 * passes the first argument in eax, so the module forwards a non-null pointer
 * to the run-time `free` and returns at once for a null pointer.
 *
 * The rebuilt object holds the same instruction sequence.  The displacement of
 * the tail jump is a link-time value, because the callee moves with the link.
 */
extern void nfree(void *p);

void free(void *p)
{
    if (p) {
        nfree(p);
    }
}
