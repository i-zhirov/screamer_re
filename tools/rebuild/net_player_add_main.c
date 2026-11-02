/* The test harness for the rebuilt `net_player_add` module.
 *
 * The harness holds the 0x38-byte record array, the two cell sources, the name
 * source, the context pointer, the lookup table, and the two cell targets.  The
 * slot stub returns the chosen record index.  The default stub fills the
 * 0x10-byte block and records its address.  The register stub records its five
 * arguments.  The harness runs three paths:
 *
 *     0  the slot is 0, the name is `AB`, the context values are set, and the
 *        lookup index is 2
 *     1  the slot is 1, so the 0x38-byte stride and the index store are
 *        exercised, the lookup index is 0
 *     2  the slot is 2, the name holds 8 characters, and the field stores run
 *        after the copy, so the name cannot overwrite the two cell stores
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void net_player_add(int id);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct net_player {
    int field_00;
    int field_04;
    int field_08;
    char name[4];
    int field_10;
    int field_14;
    int field_18;
    int field_1c;
    int field_20;
    int field_24;
    int field_28;
    int field_2c;
    int field_30;
    int field_34;
};

struct context {
    int pad_a[6];
    int field_18;
    int pad_b[3];
    int field_28;
};

int DAT_001203c4;
int DAT_001203c8;
char DAT_001203cc[0x20];
int *DAT_00102dec;
int DAT_001021e4[4];
int DAT_0011d274;
int DAT_0011d230;
struct net_player DAT_0012099c[4];

static char text[8];
static struct context context;
static int slot_result;
static int default_calls;
static void *default_block;
static int register_calls;
static void *register_record;
static int register_size;
static char *register_name;
static int register_value;
static int register_count;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void copy_string(char *destination, char *source)
{
    while (*source != 0) {
        *destination = *source;
        destination = destination + 1;
        source = source + 1;
    }
    *destination = 0;
}

static int same_string(char *left, char *right)
{
    while (*left == *right) {
        if (*left == 0) {
            return 1;
        }
        left = left + 1;
        right = right + 1;
    }
    return 0;
}

int net_player_slot(void)
{
    return slot_result;
}

int __cdecl net_player_default(void *block)
{
    int index;
    unsigned char *bytes;

    default_block = block;
    bytes = (unsigned char *)block;
    for (index = 0; index < 0x10; ++index) {
        bytes[index] = (unsigned char)(0xa0 + index);
    }
    default_calls = default_calls + 1;
    return 1;
}

int __cdecl net_player_register(void *record, int size, char *name, int value, int count)
{
    register_record = record;
    register_size = size;
    register_name = name;
    register_value = value;
    register_count = count;
    register_calls = register_calls + 1;
    return 1;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 4; ++index) {
        ((int *)&DAT_0012099c[index])[0] = 0;
        ((int *)&DAT_0012099c[index])[1] = 0;
        ((int *)&DAT_0012099c[index])[2] = 0;
        ((int *)&DAT_0012099c[index])[3] = 0;
        ((int *)&DAT_0012099c[index])[4] = 0;
        ((int *)&DAT_0012099c[index])[5] = 0;
        ((int *)&DAT_0012099c[index])[6] = 0;
        ((int *)&DAT_0012099c[index])[7] = 0;
        ((int *)&DAT_0012099c[index])[8] = 0;
        ((int *)&DAT_0012099c[index])[9] = 0;
        ((int *)&DAT_0012099c[index])[10] = 0;
        ((int *)&DAT_0012099c[index])[11] = 0;
        ((int *)&DAT_0012099c[index])[12] = 0;
        ((int *)&DAT_0012099c[index])[13] = 0;
    }
    context.field_18 = 0;
    context.field_28 = 0;
    DAT_00102dec = &context.field_18 - 6;
    slot_result = 0;
    default_calls = 0;
    default_block = 0;
    register_calls = 0;
    register_record = 0;
    register_size = 0;
    register_name = 0;
    register_value = 0;
    register_count = 0;
    DAT_001203c4 = 0x11111111;
    DAT_001203c8 = 0x22222222;
    DAT_0011d274 = 0x33333333;
    DAT_0011d230 = 0x44444444;
    DAT_001021e4[0] = 0x55550000;
    DAT_001021e4[1] = 0x55550001;
    DAT_001021e4[2] = 0x55550002;
    DAT_001021e4[3] = 0x55550003;
    copy_string(DAT_001203cc, "");
}

static int default_block_is(int record)
{
    return default_block == (void *)&DAT_0012099c[record].field_28;
}

static int record_is(int index, int id, int lookup, char *name)
{
    struct net_player *record;

    record = &DAT_0012099c[index];
    return record->field_04 == id
        && record->field_08 == index
        && same_string(record->name, name)
        && record->field_10 == DAT_001203c4
        && record->field_14 == DAT_001203c8
        && record->field_1c == 0x13572468
        && record->field_18 == DAT_001021e4[lookup]
        && record->field_20 == DAT_0011d274
        && record->field_24 == DAT_0011d230
        && default_calls == 1
        && default_block_is(index)
        && register_calls == 1
        && register_record == (void *)record
        && register_size == 0xc
        && register_name == record->name
        && register_value == 0x74
        && register_count == 9;
}

int main(void)
{
    reset();
    slot_result = 0;
    context.field_18 = 0x13572468;
    context.field_28 = 2;
    copy_string(DAT_001203cc, "AB");
    net_player_add(5);
    put(record_is(0, 5, 2, "AB") ? '0' : 'x');

    reset();
    slot_result = 1;
    context.field_18 = 0x13572468;
    context.field_28 = 0;
    copy_string(DAT_001203cc, "");
    net_player_add(-7);
    put(record_is(1, -7, 0, "") ? '1' : 'x');

    reset();
    slot_result = 2;
    context.field_18 = 0x13572468;
    context.field_28 = 3;
    copy_string(DAT_001203cc, "LONGNAME");
    net_player_add(9);
    put(record_is(2, 9, 3, "LONGNAME") ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
