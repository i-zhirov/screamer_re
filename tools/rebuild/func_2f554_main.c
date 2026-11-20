/* The test harness for the rebuilt `func_2f554` module.
 *
 * The harness holds the resource record and the four record slots.  It runs
 * four paths:
 *
 *     0  the count 3 with the requested sizes 0x10, 0x20, and 0x04: the total
 *        0x64
 *     1  the count 0: the total 0
 *     2  the count 1 with the requested size 0: the total 0x10
 *     3  the count 2 with the requested sizes 0x1234 and 0x20: the total 0x1274
 *     D  the exit marker
 *
 * Every path keeps the record array pointer, the count, the limit, and the
 * status in place.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

struct resource_record {
    char name[0xc];
    void *allocation;
    unsigned int address;
    int requested;
    int flag;
};

struct resource {
    struct resource_record *records;
    int count;
    int limit;
    int status;
};

int func_2f554(struct resource *resource);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static struct resource_record records[4];
static struct resource resource;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(void)
{
    int index;

    for (index = 0; index < (int)sizeof(records); index++) {
        ((unsigned char *)records)[index] = 0xaa;
    }
    resource.records = records;
    resource.count = 0;
    resource.limit = 0x1234;
    resource.status = 0x5678;
}

int main(void)
{
    int total;

    reset();
    records[0].requested = 0x10;
    records[1].requested = 0x20;
    records[2].requested = 0x04;
    resource.count = 3;
    total = func_2f554(&resource);
    put(total == 0x64
        && resource.count == 3
        && resource.limit == 0x1234
        && resource.status == 0x5678
        && resource.records == records
        ? '0' : 'x');

    reset();
    resource.count = 0;
    total = func_2f554(&resource);
    put(total == 0
        && resource.count == 0
        && resource.limit == 0x1234
        && resource.status == 0x5678
        ? '1' : 'x');

    reset();
    records[0].requested = 0;
    resource.count = 1;
    total = func_2f554(&resource);
    put(total == 0x10
        && resource.count == 1
        && resource.limit == 0x1234
        && resource.status == 0x5678
        ? '2' : 'x');

    reset();
    records[0].requested = 0x1234;
    records[1].requested = 0x20;
    resource.count = 2;
    total = func_2f554(&resource);
    put(total == 0x1274
        && resource.count == 2
        && resource.limit == 0x1234
        && resource.status == 0x5678
        ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
