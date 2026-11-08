/* The rebuilt `traffic_step` module.
 *
 * The original sits at linear 0x1eae0 in STARTH.EXE and holds 112 instructions
 * and 358 bytes up to the `ret` at 0x1ec45.  The next function starts at
 * 0x1ec48, so the store record keeps the 360-byte extent and sets no separate
 * function size.  The identity check uses 112 instructions.
 *
 * The caller passes the retry limit in eax.  The race finish update at 0x21058
 * passes 3.  The race session start at 0x128e0, the mode-10 start at 0x12d0c,
 * the traffic view group at 0x201d0, and the options screen at 0x21338 pass
 * 0x50 or 3.
 *
 * The function clears the finished field at +0x60 of each 0x2a8-byte traffic
 * record.  It resets the traffic table through 0x1ec7c and then waits for the
 * records to finish.  A record counts as open when its state at +0x00 is not
 * negative and its finished field is zero.  A record with flag bit 1 set at
 * +0x16 takes the state -3.
 *
 * When a record stays open, the function repeats the wait.  After the retry
 * limit it marks the last open record as -2, binds the two objects at +0x64 and
 * +0x6c through 0x11850, and restores the saved phase at 0x7f9.  The frame
 * counter at 0x7fd drives the timeout through 0x1fa04, and 0x1eed8 reads the
 * next traffic event.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern int DAT_000007fd;
extern int DAT_000007f9;
extern int DAT_0000634c;
extern int DAT_00015218;
extern char DAT_00113cd8[];

extern unsigned short traffic_elapsed(int previous, int current);
extern void traffic_reset(void);
extern void traffic_link(void);
extern int traffic_bind(int table, int state, void *slot);

struct traffic_record {
    int state;
    unsigned char pad0[0x12];
    unsigned char flags;
    unsigned char pad1[0x49];
    int finished;
    int body_slot;
    int pad2;
    int part_slot;
    unsigned char pad3[0x238];
};

void traffic_step(int limit)
{
    struct traffic_record *records;
    struct traffic_record *record;
    struct traffic_record *end;
    int previous;
    int saved;
    int tries;
    int flag;
    unsigned short elapsed;
    unsigned int threshold;

    records = (struct traffic_record *)DAT_00113cd8;

    for (DAT_0000634c = 0; DAT_0000634c < DAT_00015218; DAT_0000634c++) {
        records[DAT_0000634c].finished = 0;
    }

    traffic_reset();
    previous = DAT_000007fd;
    tries = 0;
    saved = DAT_000007f9;
    record = records;

    for (;;) {
        threshold = (unsigned int)((DAT_000007fd & 3) + 0x50);
        elapsed = traffic_elapsed(previous, DAT_000007fd);
        if ((unsigned int)elapsed > threshold) {
            traffic_reset();
            previous = DAT_000007fd;
            tries++;
            if (tries > limit) {
                record->state = -2;
                traffic_bind(0x6530, 0x6350, &record->body_slot);
                traffic_bind(0x6530, 0x6350, &record->part_slot);
                DAT_000007f9 = saved;
            }
        }

        traffic_link();
        flag = 0;
        record = records;
        end = records + DAT_00015218;
        while (record < end) {
            if (record->state >= 0) {
                if ((record->flags & 2) != 0) {
                    record->state = -3;
                }
                if (record->finished == 0) {
                    flag = 1;
                    break;
                }
            }
            record++;
        }

        if (flag == 0 || limit == 0) {
            return;
        }
    }
}
