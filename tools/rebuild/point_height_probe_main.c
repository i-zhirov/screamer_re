/* The test harness for the rebuilt `point_height_probe` module.
 *
 * The harness holds one shape block with the entry index table and up to three
 * polygon entries, one object, one record, and the two callee stubs.  The stubs
 * follow a small return script and record the polygon entry and the record that
 * they receive.
 *
 * The harness runs eight paths:
 *
 *     0  one three-vertex entry misses: the call returns 0 after one tri call
 *     1  one four-vertex entry hits: the call returns 1 and stores the height 28
 *        and the three entry words
 *     2  the first entry hits with a zero divisor and the second entry hits: the
 *        loop skips the first entry and stores the second
 *     3  two entries miss: the call returns 0 after one tri call and one quad
 *        call
 *     4  the negation runs before a signed division: the height is 1002
 *     5  the count word 5 selects the tri probe: the height is 40
 *     6  three entries miss, miss, and hit: the loop reaches the third entry
 *     7  two entries hit with a zero divisor: the call returns 0 and stores
 *        neither the height nor the words
 *     D  the exit marker
 */
#pragma off (check_stack)

int point_height_probe(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char *DAT_000c740;
char *DAT_000c744;
char *DAT_000c748;
int *DAT_000c71c;
int DAT_000c714;
int DAT_000c718;
char *DAT_000c720;
char *DAT_000c724;
int DAT_000c728;
int DAT_000c72c;
int DAT_000c730;
int DAT_000c734;
int DAT_000c738;
int DAT_000c73c;

static char text[8];
static unsigned char shape_buf[0x100];
static unsigned char object_buf[0x40];
static unsigned char record_buf[0x40];
static int tri_returns[8];
static int quad_returns[8];
static int tri_calls;
static int quad_calls;
static void *tri_poly[8];
static void *quad_poly[8];
static void *tri_record[8];
static void *quad_record[8];

int tri_point_test(void *poly, void *record)
{
    int value;

    value = 0;
    if (tri_calls < 8) {
        value = tri_returns[tri_calls];
        tri_poly[tri_calls] = poly;
        tri_record[tri_calls] = record;
    }
    tri_calls++;
    return value;
}

int quad_point_test(void *poly, void *record)
{
    int value;

    value = 0;
    if (quad_calls < 8) {
        value = quad_returns[quad_calls];
        quad_poly[quad_calls] = poly;
        quad_record[quad_calls] = record;
    }
    quad_calls++;
    return value;
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

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

static void clear_bytes(unsigned char *base, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        base[index] = 0;
    }
}

static void reset_probe(void)
{
    clear_bytes(shape_buf, sizeof shape_buf);
    tri_calls = 0;
    quad_calls = 0;
    DAT_000c740 = (char *)shape_buf;
    DAT_000c744 = (char *)record_buf;
    DAT_000c748 = (char *)object_buf;
    DAT_000c71c = 0;
    DAT_000c714 = 0x22222222;
    DAT_000c718 = 0;
    DAT_000c720 = 0;
    DAT_000c724 = 0;
    DAT_000c728 = 0;
    DAT_000c72c = 0x11111111;
    DAT_000c730 = 0;
    DAT_000c734 = 0;
    DAT_000c738 = 0;
    DAT_000c73c = 0;
}

static void set_shape(int x, int height, int y, int count,
                      int vertex_offset, int entry_offset)
{
    *(int *)(shape_buf + 0x00) = x;
    *(int *)(shape_buf + 0x04) = height;
    *(int *)(shape_buf + 0x08) = y;
    *(int *)(shape_buf + 0x14) = count;
    *(int *)(shape_buf + 0x1c) = vertex_offset;
    *(int *)(shape_buf + 0x28) = entry_offset;
}

static void set_table(int entry_offset, int index, unsigned short value)
{
    *(unsigned short *)(shape_buf + entry_offset + index * 2) = value;
}

static void set_entry(int offset, short vertex_count, int field14,
                      int field18, int field1c)
{
    *(short *)(shape_buf + offset + 0x06) = vertex_count;
    *(int *)(shape_buf + offset + 0x14) = field14;
    *(int *)(shape_buf + offset + 0x18) = field18;
    *(int *)(shape_buf + offset + 0x1c) = field1c;
}

int main(void)
{
    int result;
    int ok;

    reset_probe();
    set_shape(0, 0, 0, 1, 0x20, 0x40);
    set_table(0x40, 0, 0x80);
    set_entry(0x80, 3, 0, 0, 0);
    tri_returns[0] = 0;
    DAT_000c728 = 100;
    DAT_000c730 = 200;
    result = point_height_probe();
    ok = result == 0
        && tri_calls == 1
        && quad_calls == 0
        && tri_poly[0] == (void *)(shape_buf + 0x80)
        && tri_record[0] == (void *)record_buf
        && DAT_000c714 == 1
        && DAT_000c718 == 1
        && DAT_000c71c == (int *)(object_buf + 0x20)
        && DAT_000c720 == (char *)(shape_buf + 0x80)
        && DAT_000c724 == (char *)(shape_buf + 0x40)
        && DAT_000c72c == 0x11111111
        && DAT_000c734 == 0
        && DAT_000c738 == 0
        && DAT_000c73c == 0;
    report(ok, '0');

    reset_probe();
    set_shape(0, 100, 0, 1, 0x20, 0x40);
    set_table(0x40, 0, 0x80);
    set_entry(0x80, 4, 5, 7, 9);
    quad_returns[0] = 1;
    DAT_000c728 = 30;
    DAT_000c730 = 40;
    result = point_height_probe();
    ok = result == 1
        && tri_calls == 0
        && quad_calls == 1
        && quad_poly[0] == (void *)(shape_buf + 0x80)
        && quad_record[0] == (void *)record_buf
        && DAT_000c72c == 28
        && DAT_000c734 == 5
        && DAT_000c738 == 7
        && DAT_000c73c == 9;
    report(ok, '1');

    reset_probe();
    set_shape(10, 500, 20, 2, 0x20, 0x40);
    set_table(0x40, 0, 0x80);
    set_table(0x40, 1, 0x90);
    set_entry(0x80, 3, 11, 0, 13);
    set_entry(0x90, 3, 21, 2, 23);
    tri_returns[0] = 1;
    tri_returns[1] = 1;
    DAT_000c728 = 14;
    DAT_000c730 = 20;
    result = point_height_probe();
    ok = result == 1
        && tri_calls == 2
        && quad_calls == 0
        && tri_poly[0] == (void *)(shape_buf + 0x80)
        && tri_poly[1] == (void *)(shape_buf + 0x90)
        && DAT_000c72c == 458
        && DAT_000c734 == 21
        && DAT_000c738 == 2
        && DAT_000c73c == 23;
    report(ok, '2');

    reset_probe();
    set_shape(0, 700, 0, 2, 0x20, 0x40);
    set_table(0x40, 0, 0x80);
    set_table(0x40, 1, 0x90);
    set_entry(0x80, 3, 0, 0, 0);
    set_entry(0x90, 4, 0, 0, 0);
    tri_returns[0] = 0;
    quad_returns[0] = 0;
    result = point_height_probe();
    ok = result == 0
        && tri_calls == 1
        && quad_calls == 1
        && DAT_000c714 == 2
        && DAT_000c718 == 2
        && DAT_000c72c == 0x11111111
        && DAT_000c734 == 0
        && DAT_000c738 == 0
        && DAT_000c73c == 0;
    report(ok, '3');

    reset_probe();
    set_shape(0, 1000, 0, 1, 0x20, 0x40);
    set_table(0x40, 0, 0x80);
    set_entry(0x80, 3, 3, -4, 5);
    tri_returns[0] = 1;
    DAT_000c728 = 7;
    DAT_000c730 = -2;
    result = point_height_probe();
    ok = result == 1
        && tri_calls == 1
        && DAT_000c72c == 1002
        && DAT_000c734 == 3
        && DAT_000c738 == -4
        && DAT_000c73c == 5;
    report(ok, '4');

    reset_probe();
    set_shape(0, 50, 0, 1, 0, 0x40);
    set_table(0x40, 0, 0x80);
    set_entry(0x80, 5, 1, 1, 0);
    tri_returns[0] = 1;
    DAT_000c728 = 10;
    DAT_000c730 = 0;
    result = point_height_probe();
    ok = result == 1
        && tri_calls == 1
        && quad_calls == 0
        && DAT_000c72c == 40;
    report(ok, '5');

    reset_probe();
    set_shape(0, 0, 0, 3, 0, 0x40);
    set_table(0x40, 0, 0x80);
    set_table(0x40, 1, 0x90);
    set_table(0x40, 2, 0xa0);
    set_entry(0x80, 3, 0, 0, 0);
    set_entry(0x90, 4, 0, 0, 0);
    set_entry(0xa0, 3, 2, 2, 4);
    tri_returns[0] = 0;
    quad_returns[0] = 0;
    tri_returns[1] = 1;
    DAT_000c728 = 6;
    DAT_000c730 = 8;
    result = point_height_probe();
    ok = result == 1
        && tri_calls == 2
        && quad_calls == 1
        && tri_poly[0] == (void *)(shape_buf + 0x80)
        && tri_poly[1] == (void *)(shape_buf + 0xa0)
        && quad_poly[0] == (void *)(shape_buf + 0x90)
        && DAT_000c714 == 2
        && DAT_000c72c == -22
        && DAT_000c734 == 2
        && DAT_000c738 == 2
        && DAT_000c73c == 4;
    report(ok, '6');

    reset_probe();
    set_shape(0, 900, 0, 2, 0, 0x40);
    set_table(0x40, 0, 0x80);
    set_table(0x40, 1, 0x90);
    set_entry(0x80, 3, 1, 0, 2);
    set_entry(0x90, 4, 3, 0, 4);
    tri_returns[0] = 1;
    quad_returns[0] = 1;
    DAT_000c728 = 5;
    DAT_000c730 = 5;
    result = point_height_probe();
    ok = result == 0
        && tri_calls == 1
        && quad_calls == 1
        && DAT_000c714 == 2
        && DAT_000c72c == 0x11111111
        && DAT_000c734 == 0
        && DAT_000c738 == 0
        && DAT_000c73c == 0;
    report(ok, '7');

    put('D');
    quit();
    return 0;
}
