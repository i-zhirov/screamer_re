/* The test harness for the rebuilt `point_hit_probe` module.
 *
 * The harness holds one shape block with the entry index table and up to three
 * polygon entries, one object, one record, and the two callee stubs.  The stubs
 * follow a small return script and record the polygon entry and the record that
 * they receive.
 *
 * The harness runs eight paths:
 *
 *     0  one three-vertex entry misses: the call returns 0 after one tri call
 *     1  one four-vertex entry hits: the call returns 1 after one quad call
 *     2  the first tri entry misses and the second tri entry hits: the call
 *        returns 1 and reaches the second entry
 *     3  two entries miss, a tri and a quad: the call returns 0 at the end
 *     4  three entries miss, miss, and hit: the call returns 1 at the third
 *     5  the count word 5 selects the tri probe: the call returns 1
 *     6  two quad entries miss and hit: the call returns 1 at the second
 *     7  a quad miss then a tri miss: the call returns 0 at the end
 *     D  the exit marker
 */
#pragma off (check_stack)

int point_hit_probe(void);
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

static void set_entry(int offset, short vertex_count)
{
    *(short *)(shape_buf + offset + 0x06) = vertex_count;
}

int main(void)
{
    int result;
    int ok;

    reset_probe();
    set_shape(0, 0, 0, 1, 0x20, 0x40);
    set_table(0x40, 0, 0x80);
    set_entry(0x80, 3);
    tri_returns[0] = 0;
    result = point_hit_probe();
    ok = result == 0
        && tri_calls == 1
        && quad_calls == 0
        && tri_poly[0] == (void *)(shape_buf + 0x80)
        && tri_record[0] == (void *)record_buf
        && DAT_000c714 == 1
        && DAT_000c718 == 1
        && DAT_000c71c == (int *)(object_buf + 0x20)
        && DAT_000c720 == (char *)(shape_buf + 0x80)
        && DAT_000c724 == (char *)(shape_buf + 0x40);
    report(ok, '0');

    reset_probe();
    set_shape(0, 100, 0, 1, 0x20, 0x40);
    set_table(0x40, 0, 0x80);
    set_entry(0x80, 4);
    quad_returns[0] = 1;
    result = point_hit_probe();
    ok = result == 1
        && tri_calls == 0
        && quad_calls == 1
        && quad_poly[0] == (void *)(shape_buf + 0x80)
        && quad_record[0] == (void *)record_buf
        && DAT_000c714 == 0
        && DAT_000c718 == 1;
    report(ok, '1');

    reset_probe();
    set_shape(10, 500, 20, 2, 0x20, 0x40);
    set_table(0x40, 0, 0x80);
    set_table(0x40, 1, 0x90);
    set_entry(0x80, 3);
    set_entry(0x90, 3);
    tri_returns[0] = 0;
    tri_returns[1] = 1;
    result = point_hit_probe();
    ok = result == 1
        && tri_calls == 2
        && quad_calls == 0
        && tri_poly[0] == (void *)(shape_buf + 0x80)
        && tri_poly[1] == (void *)(shape_buf + 0x90)
        && DAT_000c714 == 1
        && DAT_000c718 == 2;
    report(ok, '2');

    reset_probe();
    set_shape(0, 700, 0, 2, 0x20, 0x40);
    set_table(0x40, 0, 0x80);
    set_table(0x40, 1, 0x90);
    set_entry(0x80, 3);
    set_entry(0x90, 4);
    tri_returns[0] = 0;
    quad_returns[0] = 0;
    result = point_hit_probe();
    ok = result == 0
        && tri_calls == 1
        && quad_calls == 1
        && DAT_000c714 == 2
        && DAT_000c718 == 2;
    report(ok, '3');

    reset_probe();
    set_shape(0, 1000, 0, 3, 0, 0x40);
    set_table(0x40, 0, 0x80);
    set_table(0x40, 1, 0x90);
    set_table(0x40, 2, 0xa0);
    set_entry(0x80, 3);
    set_entry(0x90, 4);
    set_entry(0xa0, 3);
    tri_returns[0] = 0;
    quad_returns[0] = 0;
    tri_returns[1] = 1;
    result = point_hit_probe();
    ok = result == 1
        && tri_calls == 2
        && quad_calls == 1
        && tri_poly[0] == (void *)(shape_buf + 0x80)
        && tri_poly[1] == (void *)(shape_buf + 0xa0)
        && quad_poly[0] == (void *)(shape_buf + 0x90)
        && DAT_000c714 == 2;
    report(ok, '4');

    reset_probe();
    set_shape(0, 50, 0, 1, 0, 0x40);
    set_table(0x40, 0, 0x80);
    set_entry(0x80, 5);
    tri_returns[0] = 1;
    result = point_hit_probe();
    ok = result == 1
        && tri_calls == 1
        && quad_calls == 0;
    report(ok, '5');

    reset_probe();
    set_shape(0, 0, 0, 2, 0, 0x40);
    set_table(0x40, 0, 0x80);
    set_table(0x40, 1, 0x90);
    set_entry(0x80, 4);
    set_entry(0x90, 4);
    quad_returns[0] = 0;
    quad_returns[1] = 1;
    result = point_hit_probe();
    ok = result == 1
        && tri_calls == 0
        && quad_calls == 2
        && quad_poly[0] == (void *)(shape_buf + 0x80)
        && quad_poly[1] == (void *)(shape_buf + 0x90)
        && DAT_000c714 == 1;
    report(ok, '6');

    reset_probe();
    set_shape(0, 900, 0, 2, 0, 0x40);
    set_table(0x40, 0, 0x80);
    set_table(0x40, 1, 0x90);
    set_entry(0x80, 4);
    set_entry(0x90, 3);
    quad_returns[0] = 0;
    tri_returns[0] = 0;
    result = point_hit_probe();
    ok = result == 0
        && tri_calls == 1
        && quad_calls == 1
        && DAT_000c714 == 2;
    report(ok, '7');

    put('D');
    quit();
    return 0;
}
