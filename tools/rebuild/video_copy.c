/* The rebuilt `video_copy` module.
 *
 * The original sits at linear 0x8beb8 in STARTH.EXE and holds 143 instructions
 * and 468 bytes up to the `ret` at 0x8c08b.  The next function starts at
 * 0x8c0fc, so the store record keeps the 580-byte extent and sets no separate
 * function size.  The identity check uses 143 instructions.
 *
 * The only direct caller is `video_fill` at 0x2aa98.  It passes the destination
 * height 480 in eax, the source height 400 in edx, and the source buffer
 * pointer in ebx.  The caller names the function `video_copy`, so the record
 * takes that name with the confidence inferred.
 *
 * The function forms the fixed-point source row step from the two heights:
 * ((400 << 16) / 480) << 16.  For each destination row it adds the step to a
 * 32-bit accumulator and advances the source pointer by one 640-byte row on a
 * carry.  It copies 0xa0 dwords of a row, then rewinds the source pointer by
 * one row.  The source is a 640 by 400 buffer and the destination is the 640 by
 * 480 VGA framebuffer at 0xa0000, so the function scales the buffer vertically.
 *
 * The copy runs bank by bank.  Each VBE bank holds 0x10000 bytes.  The partial
 * rows at the bank boundaries keep the source and destination streams
 * contiguous: 102 rows and 0x40 dwords, then 0x60 dwords and 101 rows and 0x80
 * dwords, then 0x20 dwords and 102 rows and 0x20 dwords, then 0x80 dwords and
 * 101 rows and 0x60 dwords, then 0x40 dwords and 70 rows.  The five calls to
 * the VBE bank switch at 0x8c0fc select the five banks.
 *
 * The original has no EBP frame and uses no floating-point instruction.  The
 * module builds without `-of+` and links no FPU stub.
 */
#define SOURCE_STRIDE 0x280
#define DEST_WIDTH 0xa0
#define VIDEO_MEMORY 0xa0000

extern void video_bank_set(int bank);

void video_copy(int dest_height, int source_height, unsigned char *source)
{
    unsigned int step;
    unsigned int accumulator = 0;
    unsigned char *destination;
    int row;
    int word;

    step = (((unsigned int)source_height << 16) / (unsigned int)dest_height) << 16;

#define COPY_BLOCK(rows_value, width_value) \
    do { \
        for (row = 0; row < (rows_value); row++) { \
            unsigned int previous = accumulator; \
            accumulator = accumulator + step; \
            if (accumulator < previous) { \
                source = source + SOURCE_STRIDE; \
            } \
            for (word = 0; word < (width_value); word++) { \
                ((unsigned int *)destination)[word] = \
                    ((unsigned int *)source)[word]; \
            } \
            destination = destination + (width_value) * 4; \
            source = source + (width_value) * 4 - SOURCE_STRIDE; \
        } \
    } while (0)

    video_bank_set(0);
    destination = (unsigned char *)VIDEO_MEMORY;
    COPY_BLOCK(0x66, DEST_WIDTH);
    COPY_BLOCK(1, 0x40);

    video_bank_set(1);
    destination = (unsigned char *)VIDEO_MEMORY;
    COPY_BLOCK(1, 0x60);
    COPY_BLOCK(0x65, DEST_WIDTH);
    COPY_BLOCK(1, 0x80);

    video_bank_set(2);
    destination = (unsigned char *)VIDEO_MEMORY;
    COPY_BLOCK(1, 0x20);
    COPY_BLOCK(0x66, DEST_WIDTH);
    COPY_BLOCK(1, 0x20);

    video_bank_set(3);
    destination = (unsigned char *)VIDEO_MEMORY;
    COPY_BLOCK(1, 0x80);
    COPY_BLOCK(0x65, DEST_WIDTH);
    COPY_BLOCK(1, 0x60);

    video_bank_set(4);
    destination = (unsigned char *)VIDEO_MEMORY;
    COPY_BLOCK(1, 0x40);
    COPY_BLOCK(0x46, DEST_WIDTH);

#undef COPY_BLOCK
}
