/* The rebuilt `traffic_event_read` module.
 *
 * The original sits at linear 0x1eed8 in STARTH.EXE and holds 54 instructions
 * and 142 bytes up to the `ret` at 0x1ef65.  The next function starts at
 * 0x1ef68, so the store record keeps the 144-byte extent and sets no separate
 * function size.  The identity check uses 54 instructions.
 *
 * The only direct caller is `traffic_step` at 0x1eae0 through the call at
 * 0x1ebbb.  The caller runs this function before it scans the traffic records
 * for the finished field, so the function drains the incoming traffic events.
 *
 * The receive queue holds sixteen entries.  The dequeue helper at 0xc739c
 * copies a twelve-byte header and returns a pointer to the 0x4c-byte payload.
 * The header holds the owner at +0x04 and the car index at +0x08.  The queue
 * belongs to the race session, so the owner must equal the race signature at
 * 0x1521c.  The local player index at 0x1d250 selects the record to skip.
 *
 * A matching event takes the 0x2a8-byte traffic record at 0x13cd8 with the
 * header index.  The copy helper at 0xc8208 moves the 0x4c payload bytes to
 * record offset +0x14, the function sets the finished field at +0x60 to one,
 * and it stores the index in the state field at +0x00.  A rejected event runs
 * the queue helper at 0xc76f0 once.  The function ends when the dequeue helper
 * returns zero.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so the module uses no FPU stub
 * and no assembly.  The three callee names stay local to the harness.
 */
#define TRAFFIC_RECORD_SIZE 0x2a8
#define TRAFFIC_STATE_OFFSET 0x14
#define TRAFFIC_STATE_COPY 0x4c
#define TRAFFIC_FINISHED_OFFSET 0x60
#define EVENT_HEADER_SIZE 0x0c

struct event_header {
    int sequence;               /* +0x00 */
    int owner;                  /* +0x04 */
    int index;                  /* +0x08 */
};

struct traffic_record {
    int state;                  /* +0x00 */
    unsigned char pad0[0x5c];   /* +0x04 */
    int finished;               /* +0x60 */
    unsigned char pad1[0x244];  /* +0x64 */
};

extern int DAT_0011521c;
extern int DAT_0011d250;
extern char DAT_00113cd8[];

extern int traffic_event_dequeue(
    struct event_header *header, int length, void **payload
);
extern void traffic_event_copy(void *source, void *destination, int count);
extern void traffic_event_pump(void);

void traffic_event_read(void)
{
    struct event_header header;
    struct traffic_record *record;
    void *payload;

    for (;;) {
        if (traffic_event_dequeue(&header, EVENT_HEADER_SIZE, &payload) == 0) {
            break;
        }
        if (header.owner == DAT_0011521c && header.index != DAT_0011d250) {
            record = (struct traffic_record *)(
                DAT_00113cd8 + header.index * TRAFFIC_RECORD_SIZE
            );
            traffic_event_copy(
                payload,
                (char *)record + TRAFFIC_STATE_OFFSET,
                TRAFFIC_STATE_COPY
            );
            record->finished = 1;
            record->state = header.index;
        } else {
            traffic_event_pump();
        }
    }
}
