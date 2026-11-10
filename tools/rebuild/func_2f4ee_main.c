/* The test harness for the rebuilt `func_2f4ee` module.
 *
 * The harness holds the resource record and the four record slots.  The `free`
 * stub records the pointer of every call, and the `memset` stub records its
 * block, value, and size.
 *
 * The harness runs four paths:
 *
 *     0  the count 3 with three allocation results: the three frees in order,
 *        one memset of 0x54 bytes, the count 0, and the untouched tail
 *     1  the count 0: no free, one memset of size 0, and the array unchanged
 *     2  the count 1 with one allocation result: one free, one memset of 0x1c
 *        bytes, and the count 0
 *     3  the count 2 with a null and a set allocation result: two frees in
 *        order, one memset of 0x38 bytes, and the count 0
 *     D  the exit marker
 *
 * Every path keeps the record array pointer, the limit at `+0x08`, and the
 * status at `+0x0c` in place.
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

void func_2f4ee(struct resource *resource);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static struct resource_record records[4];
static struct resource resource;
static unsigned char blocks[4][0x10];

static void *free_arguments[8];
static int free_calls;

static void *memset_block;
static int memset_value;
static unsigned int memset_size;
static int memset_calls;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void free(void *block)
{
    free_arguments[free_calls] = block;
    free_calls = free_calls + 1;
}

void *memset(void *block, int value, unsigned int size)
{
    unsigned char *bytes = (unsigned char *)block;
    unsigned int index;

    memset_block = block;
    memset_value = value;
    memset_size = size;
    memset_calls = memset_calls + 1;
    for (index = 0; index < size; index++) {
        bytes[index] = (unsigned char)value;
    }
    return block;
}

static int all_bytes(void *target, unsigned int size, unsigned char value)
{
    unsigned char *bytes = (unsigned char *)target;
    unsigned int index;

    for (index = 0; index < size; index++) {
        if (bytes[index] != value) {
            return 0;
        }
    }
    return 1;
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

    free_calls = 0;
    for (index = 0; index < 8; index++) {
        free_arguments[index] = 0;
    }
    memset_block = 0;
    memset_value = -1;
    memset_size = 0;
    memset_calls = 0;
}

int main(void)
{
    reset();
    records[0].allocation = &blocks[0];
    records[1].allocation = &blocks[1];
    records[2].allocation = &blocks[2];
    resource.count = 3;
    func_2f4ee(&resource);
    put(free_calls == 3
        && free_arguments[0] == (void *)&blocks[0]
        && free_arguments[1] == (void *)&blocks[1]
        && free_arguments[2] == (void *)&blocks[2]
        && memset_calls == 1
        && memset_block == (void *)records
        && memset_value == 0
        && memset_size == 0x54
        && resource.count == 0
        && resource.limit == 0x1234
        && resource.status == 0x5678
        && resource.records == records
        && all_bytes(records, 0x54, 0)
        && all_bytes((unsigned char *)records + 0x54, 0x70 - 0x54, 0xaa)
        ? '0' : 'x');

    reset();
    resource.count = 0;
    func_2f4ee(&resource);
    put(free_calls == 0
        && memset_calls == 1
        && memset_block == (void *)records
        && memset_value == 0
        && memset_size == 0
        && resource.count == 0
        && resource.limit == 0x1234
        && resource.status == 0x5678
        && resource.records == records
        && all_bytes(records, sizeof(records), 0xaa)
        ? '1' : 'x');

    reset();
    records[0].allocation = &blocks[0];
    resource.count = 1;
    func_2f4ee(&resource);
    put(free_calls == 1
        && free_arguments[0] == (void *)&blocks[0]
        && memset_calls == 1
        && memset_size == 0x1c
        && resource.count == 0
        && all_bytes(records, 0x1c, 0)
        && all_bytes((unsigned char *)records + 0x1c, 0x70 - 0x1c, 0xaa)
        ? '2' : 'x');

    reset();
    records[0].allocation = 0;
    records[1].allocation = &blocks[1];
    resource.count = 2;
    func_2f4ee(&resource);
    put(free_calls == 2
        && free_arguments[0] == 0
        && free_arguments[1] == (void *)&blocks[1]
        && memset_calls == 1
        && memset_size == 0x38
        && resource.count == 0
        && all_bytes(records, 0x38, 0)
        && all_bytes((unsigned char *)records + 0x38, 0x70 - 0x38, 0xaa)
        ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
