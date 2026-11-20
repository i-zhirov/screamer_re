/* The rebuilt `data_blit` module.
 *
 * The original sits at linear 0x102b0 in STARTH.EXE and holds 26 instructions
 * and 52 bytes up to the `ret` at 0x102e3.  The next entry starts at 0x102e4,
 * so the store record keeps the 52-byte extent and sets no separate function
 * size.  The identity check uses 26 instructions.
 *
 * The gauge panel at 0x2369c calls this function before each cockpit blit and
 * passes the compressed source in `eax` and the destination in `edx`.  The
 * source is a sequence of signed 32-bit spans.  A positive span copies that
 * many literal bytes, a negative span moves the destination pointer forward
 * without a write, and a zero span ends the sequence.
 *
 * The caller and the span format support the name `data_blit`.  The original
 * has no symbol, so the record keeps the name with the confidence inferred.
 * The function sits with the program functions, so the record takes the
 * section Program functions.
 *
 * The original has no EBP frame and saves every register through `pushal`.  It
 * holds no floating-point instruction and makes no call, so the module needs
 * no FPU stub and no assembly body.  The module implementation is C.
 */
void data_blit(void *source, void *destination)
{
    unsigned char *src = (unsigned char *)source;
    unsigned char *dst = (unsigned char *)destination;
    int length;

    length = *(int *)src;
    src += 4;

    while (length != 0) {
        if (length < 0) {
            dst -= length;
        } else {
            int count = length;
            int words = count >> 2;

            while (words != 0) {
                *(unsigned long *)dst = *(unsigned long *)src;
                dst += 4;
                src += 4;
                words--;
            }
            count &= 3;
            while (count != 0) {
                *dst = *src;
                dst++;
                src++;
                count--;
            }
        }
        length = *(int *)src;
        src += 4;
    }
}
