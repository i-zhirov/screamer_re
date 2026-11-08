/* The test harness for the rebuilt `video_copy` module.
 *
 * The harness holds the 640 by 400 source buffer, the five captured VBE banks,
 * and the `video_bank_set` stub.  The stub records the bank order and copies
 * the 64 KiB window at 0xa0000 into the matching capture slot.  The module
 * writes each bank without a real bank switch, so each capture holds the bytes
 * that the module wrote for that bank.
 *
 * The source holds a byte pattern that depends on the source index, so a wrong
 * source row shows as a different byte.  The harness rebuilds the expected
 * output from the original row step ((400 << 16) / 480) << 16 and compares all
 * 307200 destination bytes.  It prints:
 *
 *     0  the stub saw the five banks in order
 *     1  the destination holds the expected vertically scaled source
 *     D  the exit marker
 */
#pragma off (check_stack)

#define SOURCE_ROWS 400
#define SOURCE_WIDTH 640
#define DEST_ROWS 480
#define BANK_BYTES 0x10000
#define BANK_COUNT 5
#define LAST_BANK_BYTES (DEST_ROWS * SOURCE_WIDTH - (BANK_COUNT - 1) * BANK_BYTES)

void video_copy(int dest_height, int source_height, unsigned char *source);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char source[SOURCE_ROWS * SOURCE_WIDTH];
static unsigned char banks[BANK_COUNT][BANK_BYTES];
static int bank_count;
static int bank_order_ok = 1;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void video_bank_set(int bank)
{
    unsigned char *video = (unsigned char *)0xa0000;
    int index;

    if (bank != bank_count) {
        bank_order_ok = 0;
    }
    if (bank > 0 && bank < BANK_COUNT) {
        for (index = 0; index < BANK_BYTES; index++) {
            banks[bank - 1][index] = video[index];
        }
    }
    bank_count = bank_count + 1;
}

int main(void)
{
    unsigned char *video = (unsigned char *)0xa0000;
    static const unsigned char piece_rows[] = {
        0x66, 1, 1, 0x65, 1, 1, 0x66, 1, 1, 0x65, 1, 1, 0x46
    };
    static const unsigned char piece_width[] = {
        0xa0, 0x40, 0x60, 0xa0, 0x80, 0x20,
        0xa0, 0x20, 0x80, 0xa0, 0x60, 0x40, 0xa0
    };
    unsigned int step;
    unsigned int accumulator = 0;
    int source_offset = 0;
    int destination_offset = 0;
    int piece;
    int row;
    int byte;
    int mismatch = 0;
    int index;

    bank_count = 0;
    bank_order_ok = 1;
    for (index = 0; index < SOURCE_ROWS * SOURCE_WIDTH; index++) {
        source[index] = (unsigned char)(index * 31 + 7);
    }

    video_copy(DEST_ROWS, SOURCE_ROWS, source);

    for (index = 0; index < LAST_BANK_BYTES; index++) {
        banks[BANK_COUNT - 1][index] = video[index];
    }

    step = (((unsigned int)SOURCE_ROWS << 16) / (unsigned int)DEST_ROWS) << 16;
    for (piece = 0; piece < sizeof(piece_rows); piece++) {
        int width = piece_width[piece] * 4;
        for (row = 0; row < piece_rows[piece]; row++) {
            unsigned int previous = accumulator;
            accumulator = accumulator + step;
            if (accumulator < previous) {
                source_offset = source_offset + SOURCE_WIDTH;
            }
            for (byte = 0; byte < width; byte++) {
                int bank = destination_offset >> 16;
                int within = destination_offset & 0xffff;
                int source_index = source_offset + byte;
                if (source_index < 0
                    || source_index >= SOURCE_ROWS * SOURCE_WIDTH
                    || destination_offset >= DEST_ROWS * SOURCE_WIDTH) {
                    mismatch = 1;
                } else if (banks[bank][within] != source[source_index]) {
                    mismatch = 1;
                }
                destination_offset = destination_offset + 1;
            }
            source_offset = source_offset + width - SOURCE_WIDTH;
        }
    }

    put(bank_order_ok && bank_count == BANK_COUNT ? '0' : 'x');
    put(!mismatch ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
