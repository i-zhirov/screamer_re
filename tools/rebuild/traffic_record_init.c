/* The rebuilt `traffic_record_init` module.
 *
 * The original sits at linear 0x14970 in STARTH.EXE.  The first `ret` sits at
 * 0x149d1, so the body up to that early return holds 37 instructions and 98
 * bytes.  The full body continues to the `ret` at 0x14d2b, and the next
 * function starts at 0x14d2c, so the store record keeps the 956-byte extent and
 * sets no separate function size.  The identity check uses 37 instructions.
 *
 * The only direct caller is `game_mode_10_start` at 0x12d0c, which prepares
 * every traffic record through this function with the shared counter.  The
 * caller takes the section The timing and filter layer, so this module takes
 * the same section.  The caller and the findings already hold the name of the
 * record init at 0x14970 with the confidence inferred.
 *
 * The traffic record is 0x2a8 bytes.  It holds:
 *
 *     +0x00    int flags; a negative value skips the whole init
 *     +0x18    int ready; cleared at the end
 *     +0x1c    int model; a value above 0xc fails with -1
 *     +0x64    int model_slot; the first object load writes the slot
 *     +0x6c    int shadow_slot; the second object load writes the slot
 *     +0x8c    four 0x54-byte wheel blocks
 *     +0x1c4   four 0x2c-byte bound blocks; they overlap the tail of the
 *              last wheel block, which stays unused
 *
 * A wheel block holds the edge value at +0x00, the range value at +0x08, the
 * first position block at +0x2c, and the second position block at +0x34.  A
 * position block holds the object slot at +0x00 and takes the chunk index at
 * +0x04.  A bound block holds the copied edge value at +0x00 and the adjusted
 * range value at +0x08.
 *
 * The function loads the model object named by the 0x40-byte model record at
 * 0xd25c + model * 0x40 through 0x11060, and then loads the `SHADOW` object at
 * 0x5fc through the same helper.  A non-zero load result returns at once.  The
 * block object of the model slot then takes the model record field at +0x0c
 * at block offset +0x20.
 *
 * For each of the four wheels, the function finds the two chunk names at
 * 0x1d5c + index * 8 through 0x11410, reads the chunk fields at +0x14 and
 * +0x1c from the first chunk, copies the edge value to the bound block, and
 * adds 150 to an odd wheel or -250 to an even wheel for the bound range.
 *
 * The last block forms two centers from the rear wheel ranges: the first adds
 * a quarter of their difference to the first range, and the second adds a
 * quarter of the opposite difference to the second range.  The span takes the
 * absolute half difference.  The function clears the ready word and returns
 * zero.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function uses no floating-point instruction and links no FPU stub.
 *
 * The rebuild covers the full body.  The identity check uses the 37-instruction
 * window up to the early `ret`, and the behavior harness covers the whole
 * body.
 */
typedef struct traffic_position {
    int slot;
    int chunk;
} traffic_position;

typedef struct traffic_wheel {
    int edge_value;             /* +0x00 */
    int reserved_04;
    int range_value;            /* +0x08 */
    int reserved_0c[8];
    traffic_position first;     /* +0x2c */
    traffic_position second;    /* +0x34 */
    int reserved_3c[6];
} traffic_wheel;

typedef struct traffic_bound {
    int edge;                   /* +0x00 */
    int reserved_04;
    int limit;                  /* +0x08 */
    int reserved_0c[8];
} traffic_bound;

typedef struct traffic_record {
    int flags;                  /* +0x00 */
    int reserved_04[5];
    int ready;                  /* +0x18 */
    int model;                  /* +0x1c */
    int reserved_20[0x11];
    int model_slot;             /* +0x64 */
    int reserved_68;
    int shadow_slot;            /* +0x6c */
    int reserved_70[7];
    union {
        traffic_wheel wheel[4]; /* +0x8c */
        struct {
            int lead[0x4e];
            traffic_bound bound[4]; /* +0x1c4 */
            int zero_a;         /* +0x274 */
            int reserved_278;
            int center_a;       /* +0x27c */
            int zero_b;         /* +0x280 */
            int reserved_284;
            int center_b;       /* +0x288 */
            int reserved_28c[6];
            int span;           /* +0x2a4 */
        } view;
    } body;
} traffic_record;

typedef struct table_entry {
    int reserved_00[3];
    void *block;                /* +0x0c */
    void *chunks;               /* +0x10 */
    int reserved_14[2];
} table_entry;

typedef struct chunk_record {
    int reserved_00[5];
    int edge;                   /* +0x14 */
    int reserved_18;
    int range;                  /* +0x1c */
} chunk_record;

extern char DAT_00006530[];
extern char DAT_0000d25c[];

extern int object_load_named(void *table, char *name, int *slot);
extern int object_chunk_find(void *table, char *name,
                             traffic_position *position);

int traffic_record_init(traffic_record *record)
{
    table_entry *entry;
    chunk_record *chunks;
    int difference;
    int i;
    int result;
    int offset;

    if (record->flags < 0) {
        return 0;
    }
    if (record->model > 0xc) {
        return -1;
    }
    result = object_load_named((void *)0x6530,
                               (char *)(0xd25c + (record->model << 6)),
                               &record->model_slot);
    if (result != 0) {
        return result;
    }
    result = object_load_named((void *)0x6530, (char *)0x5fc,
                               &record->shadow_slot);
    if (result != 0) {
        return result;
    }

    entry = (table_entry *)(DAT_00006530 + record->model_slot * 0x1c);
    ((int *)entry->block)[8] =
        *(int *)(DAT_0000d25c + (record->model << 6) + 0x0c);

    for (i = 0; i < 4; i++) {
        record->body.wheel[i].first.slot = record->model_slot;
        record->body.wheel[i].second.slot = record->model_slot;
        object_chunk_find((void *)0x6530, (char *)(0x1d5c + i * 8),
                          &record->body.wheel[i].first);
        object_chunk_find((void *)0x6530, (char *)(0x1d60 + i * 8),
                          &record->body.wheel[i].second);
        chunks = (chunk_record *)
            *(void **)(DAT_00006530 + record->body.wheel[i].first.slot * 0x1c
                       + 0x10);
        record->body.wheel[i].edge_value =
            chunks[record->body.wheel[i].first.chunk].edge;
        record->body.wheel[i].range_value =
            chunks[record->body.wheel[i].first.chunk].range;
        record->body.view.bound[i].edge = record->body.wheel[i].edge_value;
        offset = (i & 1) ? 150 : -250;
        record->body.view.bound[i].limit =
            record->body.wheel[i].range_value + offset;
    }

    record->body.view.center_a = record->body.view.bound[2].limit
        + (record->body.view.bound[3].limit
           - record->body.view.bound[2].limit) / 4;
    record->body.view.center_b = record->body.view.bound[3].limit
        + (record->body.view.bound[2].limit
           - record->body.view.bound[3].limit) / 4;
    record->body.view.zero_b = 0;
    record->body.view.zero_a = 0;
    difference = record->body.view.center_a - record->body.view.center_b;
    difference = difference / 2;
    if (difference < 0) {
        difference = -difference;
    }
    record->body.view.span = difference;
    record->ready = 0;
    return 0;
}
