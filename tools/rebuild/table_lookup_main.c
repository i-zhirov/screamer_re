/* The test harness for the rebuilt `table_lookup` module.
 *
 * The harness holds the table flag, the table base, and the lookup stub.  The
 * stub records its arguments and returns a configured value.  The harness runs
 * the module with the flag set and then cleared:
 *
 *     0  with the flag set: the stub ran with the index and the entry
 *     1  with the flag cleared: the module returned -1
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int table_lookup(int index);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_00008058;
unsigned int DAT_00018d94;

static char text[8];
static unsigned int got_base;
static int got_index;
static unsigned int got_entry;
static int lookup_result;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int table_read(unsigned int base, int index, unsigned int entry)
{
    got_base = base;
    got_index = index;
    got_entry = entry;
    return lookup_result;
}

int main(void)
{
    int result;

    DAT_00008058 = 1;
    DAT_00018d94 = 0x1234;
    lookup_result = 0x5678;
    result = table_lookup(3);
    put(result == 0x5678 && got_base == 0x1234 && got_index == 3
        && got_entry == 0x16034 + 3 * 116 ? '0' : 'x');
    DAT_00008058 = 0;
    result = table_lookup(3);
    put(result == -1 ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
