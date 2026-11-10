/* The rebuilt `sound_source_update` module.
 *
 * The original sits at linear 0x8a958 in STARTH.EXE and holds 58 instructions
 * and 144 bytes up to the `ret` at 0x8a9e7.  The next function starts at
 * 0x8a9e8, so the store record keeps the 144-byte extent and sets no separate
 * function size.  The identity check uses 58 instructions.
 *
 * The source update runs in the shared sound service body.  `sound_update` at
 * 0x81a70 and `sound_effects_update` at 0x81ab0 store the list head at
 * 0x7451c.  The body then calls this function at 0x81ac6, reads the active
 * count 0x72488, and runs the mix prepare at 0x8aa60.  The caller passes the
 * output cursor in `edi` and ignores the returned register.  The record takes
 * the name `sound_source_update` with the confidence inferred and the section
 * Program functions.  The original has no symbol.
 *
 * The list head at 0x7451c points to an array of source pointers.  The array
 * ends at a null pointer.  An entry whose bit 0x40000000 is set is skipped,
 * because that source is already in the frame list.  The source record holds
 * the position at `+0x08`, `+0x0c`, and `+0x10`.
 *
 * The world rectangle at 0x72d94 culls the ground plane.  The function compares
 * the source x at `+0x08` with the rectangle x range at `+0x00` and `+0x0c`,
 * and the source z at `+0x10` with the rectangle second range at `+0x08` and
 * `+0x14`.  A source outside the rectangle is skipped.
 *
 * For a kept source the function forms the squared distance to the camera
 * position at 0x7a7ac + 0x0c, `+0x10`, and `+0x14`.  It shifts the 64-bit sum
 * right by two and truncates the result to 32 bits.  A value below the limit
 * 0x284af100 appends one 8-byte record to the output: the shifted sum and the
 * source pointer with the bit 0x40000000 set.  The signed compare also keeps a
 * value with the high bit set, because the original compares the truncated
 * value as a signed integer.
 *
 * At the null end the function adds the number of appended records to the
 * active count 0x72488 and returns the start of the output.  The original
 * divides the byte length by eight, so the rebuild divides the dword count by
 * two.
 *
 * The original has no EBP frame and uses no floating-point instruction, so the
 * module builds without `-of+` and links no FPU stub.  The rebuild forms the
 * signed 32x32 product and the 64-bit sum with 32-bit arithmetic, because the
 * harness links no 64-bit runtime helper.
 */
#define RANGE_LIMIT 0x284af100
#define LIST_FLAG 0x40000000

struct source {
    int field_00;
    int field_04;
    int x;
    int y;
    int z;
    int rest[11];
};

struct camera {
    int field_00;
    int field_04;
    int field_08;
    int x;
    int y;
    int z;
};

struct rect {
    int min_x;
    int reserved_04;
    int min_y;
    int max_x;
    int reserved_10;
    int max_y;
};

extern struct source **DAT_0007451c;
extern struct camera *DAT_0007a7ac;
extern struct rect DAT_00072d94;
extern int DAT_00072488;

static void square_64(int value, unsigned int *high, unsigned int *low)
{
    unsigned int a;
    unsigned int a0;
    unsigned int a1;
    unsigned int p0;
    unsigned int p1;
    unsigned int p2;
    unsigned int p3;
    unsigned int middle;
    unsigned int carry;

    a = (unsigned int)value;
    a0 = a & 0xffff;
    a1 = a >> 16;
    p0 = a0 * a0;
    p1 = a0 * a1;
    p2 = a1 * a0;
    p3 = a1 * a1;
    *low = (p1 << 16) + (p2 << 16) + p0;
    carry = ((p0 >> 16) + (p1 & 0xffff) + (p2 & 0xffff)) >> 16;
    middle = p3 + (p1 >> 16) + (p2 >> 16) + carry;
    if (value < 0) {
        middle -= a << 1;
    }
    *high = middle;
}

static int squared_metric(int dx, int dy, int dz)
{
    unsigned int low_x;
    unsigned int high_x;
    unsigned int low_y;
    unsigned int high_y;
    unsigned int low_z;
    unsigned int high_z;
    unsigned int low;
    unsigned int high;
    unsigned int sum;

    square_64(dx, &high_x, &low_x);
    square_64(dy, &high_y, &low_y);
    square_64(dz, &high_z, &low_z);
    sum = low_x + low_y;
    high = high_x + high_y + (sum < low_x ? 1u : 0u);
    low = sum + low_z;
    high = high + high_z + (low < sum ? 1u : 0u);
    return (int)((low >> 2) | (high << 30));
}

int *sound_source_update(int *output)
{
    struct source **cursor;
    struct camera *camera;
    struct source *source;
    int *start;
    int metric;

    cursor = DAT_0007451c;
    if ((int)cursor == -1) {
        return output;
    }
    start = output;
    camera = DAT_0007a7ac;
    for (;;) {
        source = *cursor++;
        if ((unsigned int)source & LIST_FLAG) {
            continue;
        }
        if (source == (struct source *)0) {
            break;
        }
        if (source->x < DAT_00072d94.min_x) {
            continue;
        }
        if (source->x > DAT_00072d94.max_x) {
            continue;
        }
        if (source->z < DAT_00072d94.min_y) {
            continue;
        }
        if (source->z > DAT_00072d94.max_y) {
            continue;
        }
        metric = squared_metric(
            source->x - camera->x,
            source->y - camera->y,
            source->z - camera->z);
        if (metric >= RANGE_LIMIT) {
            continue;
        }
        *output++ = metric;
        *output++ = (int)((unsigned int)source | LIST_FLAG);
    }
    DAT_00072488 += (int)(output - start) / 2;
    return start;
}
