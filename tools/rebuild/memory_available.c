/* The rebuilt `memory_available` module.
 *
 * The original sits at linear 0x2f8ca in STARTH.EXE and holds 27 instructions and 58
 * bytes: the six register saves `ebx`, `ecx`, `edx`, `esi`, `edi`, and `ebp`, the frame
 * `mov ebp,esp` and `sub esp,0xc`, the argument stored at `[ebp-4]`, the first call at
 * `0x2f758` with its result moved to `edx`, the second call at `0x2f82e` with the argument
 * reloaded, the sum `add edx,eax`, the sum stored at `[ebp-8]`, the return through the
 * temporary at `[ebp-0xc]`, and the epilogue.
 *
 * The function uses `edx` to hold the first result across the second call, so the prologue
 * saves six registers here against five in `list_destroy`.  The register set follows the
 * registers the function uses.
 */
extern unsigned probe_free(void *pool);
extern unsigned probe_size(void *pool);

unsigned memory_available(void *pool)
{
    unsigned total;

    total = probe_free(pool) + probe_size(pool);
    return total;
}
