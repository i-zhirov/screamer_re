/* The rebuilt `traffic_event_flush` module.
 *
 * The original sits at linear 0x1ec48 in STARTH.EXE and holds 24 instructions
 * and 50 bytes up to the `ret` at 0x1ec79.  The next function starts at
 * 0x1ec7c after the two padding bytes at 0x1ec7a, so the store record keeps the
 * 52-byte extent and sets no separate function size.  The identity check uses 24
 * instructions.
 *
 * The function drains the pending receive queue.  The dequeue helper at 0xc739c
 * copies a twelve-byte header into the local buffer and returns the payload
 * pointer.  The function discards the header and the payload, calls the queue
 * helper at 0xc76f0 once after every received message, and calls it once more
 * when the queue is empty.  The four callers use the function as the mode
 * transition wait: `game_mode_10_start` at 0x12d0c after `traffic_step`,
 * `net_race_lobby_init` at 0x2ca40 after the player record clear, and
 * `net_race_lobby` at 0x2d7c4 before the lobby loop and at 0x2da2f.
 *
 * The queue carries the race session events, so the callers, the two queue
 * helpers, and the sibling `traffic_event_read` at 0x1eed8 support the name
 * `traffic_event_flush`.  The original has no symbol, so the record keeps the
 * name with the confidence inferred.  The callers sit in several sections, so
 * the record takes the section Program functions.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction, so it needs no FPU stub and no
 * assembly.
 */
#define EVENT_HEADER_SIZE 0x0c

struct event_header {
    int sequence;               /* +0x00 */
    int owner;                  /* +0x04 */
    int index;                  /* +0x08 */
};

extern int traffic_event_dequeue(
    struct event_header *header, int length, void **payload
);
extern void traffic_event_pump(void);

void traffic_event_flush(void)
{
    struct event_header header;
    void *payload;
    int received;

    do {
        received = traffic_event_dequeue(
            &header, EVENT_HEADER_SIZE, &payload
        );
        traffic_event_pump();
    } while (received != 0);
}
