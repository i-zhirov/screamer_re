/* The test harness for the rebuilt `map_object_progress_sum` module.
 *
 * The harness holds the 0x50 map object records at `DAT_001db78`, stride
 * 0x64.  Each record holds the object id at `+0x08` and the progress value at
 * `+0x58`.  An object id of -1 marks a free record.  The function sums the
 * progress of the busy records.  Every expected sum is an exact binary value,
 * so the harness compares the values exactly.
 *
 * The harness runs these paths:
 *
 *     0  every record is free: the sum is 0.0
 *     1  every record is busy with the progress 1.0: the sum is 80.0
 *     2  one busy record at 0x20 with the progress 2.5: the sum is 2.5
 *     3  the busy records 0, 5, and 0x4f with the progress values 0.5, 0.25, and
 *        1.0: the sum is 1.75, and the progress 9.0 of the free record 7 does
 *        not count
 *     4  every record is busy with the progress index: the sum is 3160.0
 *     5  the busy records 1 and 2 with the progress -1.5 and 2.0: the sum is
 *        0.5
 *     D  the exit marker
 */
#pragma off (check_stack)

#define RECORD_SIZE 0x64
#define RECORD_COUNT 0x50
#define OBJECT_OFFSET 0x08
#define PROGRESS_OFFSET 0x58

float map_object_progress_sum(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_001db78[RECORD_SIZE * RECORD_COUNT];
static char text[8];

static int *object_at(int index)
{
    return (int *)(DAT_001db78 + index * RECORD_SIZE + OBJECT_OFFSET);
}

static float *progress_at(int index)
{
    return (float *)(DAT_001db78 + index * RECORD_SIZE + PROGRESS_OFFSET);
}

static void reset(void)
{
    int index;

    for (index = 0; index < RECORD_COUNT; index++) {
        *object_at(index) = -1;
        *progress_at(index) = 0.0f;
    }
}

static void make_busy(int index, float progress)
{
    *object_at(index) = index + 1;
    *progress_at(index) = progress;
}

static int close_enough(float value, float wanted)
{
    return value == wanted;
}

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int main(void)
{
    int index;
    int ok;

    reset();
    ok = close_enough(map_object_progress_sum(), 0.0f);
    put(ok ? '0' : 'x');

    reset();
    for (index = 0; index < RECORD_COUNT; index++)
        make_busy(index, 1.0f);
    ok = close_enough(map_object_progress_sum(), 80.0f);
    put(ok ? '1' : 'x');

    reset();
    make_busy(0x20, 2.5f);
    ok = close_enough(map_object_progress_sum(), 2.5f);
    put(ok ? '2' : 'x');

    reset();
    make_busy(0, 0.5f);
    make_busy(5, 0.25f);
    make_busy(0x4f, 1.0f);
    *progress_at(7) = 9.0f;
    ok = close_enough(map_object_progress_sum(), 1.75f);
    put(ok ? '3' : 'x');

    reset();
    for (index = 0; index < RECORD_COUNT; index++)
        make_busy(index, (float)index);
    ok = close_enough(map_object_progress_sum(), 3160.0f);
    put(ok ? '4' : 'x');

    reset();
    make_busy(1, -1.5f);
    make_busy(2, 2.0f);
    ok = close_enough(map_object_progress_sum(), 0.5f);
    put(ok ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
