/* The rebuilt `sound_source_collect` module.
 *
 * The original sits at linear 0x833a4 in STARTH.EXE and holds 98 instructions
 * and 242 bytes up to the `ret` at 0x83495.  The next function starts at
 * 0x83498, so the store record keeps the 348-byte extent and sets no separate
 * function size.  The identity check uses 98 instructions.
 *
 * The caller at 0x83354 builds the active sound list.  It takes the grid rows
 * from `sound_range_update` at 0x830c8 and calls this function once per row
 * with the row pointer in `ebp`, the output cursor in `edi`, and the row limit
 * in the global `0x734f4`.  The caller then divides the output length by the
 * 8-byte record and stores the result at the active count `0x72488`.
 * `sound_effects_update` at 0x81ab0 reaches the caller, so the record takes the
 * section Program functions.  `sound_mix_prepare` at 0x8aa60 reads the same
 * 8-byte records at 0x40330.
 *
 * The row is an array of region pointers that ends at -1.  The global
 * `0x734f4` is the row limit.  A region holds two source lists: the list at
 * `+0x04` with the count at `+0x18`, and the list at `+0x00` with the count at
 * `+0x14`.  The function walks the first list, then the second list.
 *
 * A source record is 0x40 bytes.  It holds the position at `+0x08`, `+0x0c`,
 * and `+0x10`.  For each source the function clears the field at `+0x04` and
 * forms the squared distance to the camera position at `0x7a7ac + 0x0c`,
 * `+0x10`, and `+0x14`.  It shifts the 64-bit sum right by two and truncates
 * the result to 32 bits.
 *
 * A value below the limit 0x284af100 appends one 8-byte record to the output:
 * the shifted sum and the source pointer.  The signed compare also keeps a
 * value with the high bit set, because the original truncates the shifted sum
 * to 32 bits and compares it as a signed value.
 *
 * The original has no EBP frame and uses no floating-point instruction, so the
 * module builds without `-of+` and links no FPU stub.  The rebuild forms the
 * signed 32x32 product and the 64-bit sum with 32-bit arithmetic, because the
 * harness links no 64-bit runtime helper.
 */
#define SOURCE_STRIDE 0x40
#define RANGE_LIMIT 0x284af100

struct source {
    int field_00;
    int field_04;
    int x;
    int y;
    int z;
    int rest[11];
};

struct region {
    struct source *list_0;
    struct source *list_1;
    int field_08;
    int field_0c;
    int field_10;
    int count_0;
    int count_1;
};

struct camera {
    int field_00;
    int field_04;
    int field_08;
    int x;
    int y;
    int z;
};

extern struct camera *DAT_0007a7ac;
extern int DAT_000734f4;

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

static int *collect_list(
    struct source *source, int count, int *output, struct camera *camera)
{
    int index;
    int metric;

    for (index = 0; index < count; index++) {
        source->field_04 = 0;
        metric = squared_metric(
            source->x - camera->x,
            source->y - camera->y,
            source->z - camera->z);
        if (metric < RANGE_LIMIT) {
            *output++ = metric;
            *output++ = (int)source;
        }
        source = (struct source *)((char *)source + SOURCE_STRIDE);
    }
    return output;
}

int *sound_source_collect(struct region **regions, int *output)
{
    struct camera *camera;
    struct region **cursor;
    struct region *region;
    int count;

    camera = DAT_0007a7ac;
    count = DAT_000734f4;
    cursor = regions;
    for (;;) {
        if ((unsigned int)*cursor == 0xffffffffu) {
            break;
        }
        region = (struct region *)*cursor;
        if (region->list_1 != (struct source *)0) {
            output = collect_list(
                region->list_1, region->count_1, output, camera);
        }
        output = collect_list(
            region->list_0, region->count_0, output, camera);
        cursor++;
        if (--count < 0) {
            break;
        }
    }
    return output;
}
