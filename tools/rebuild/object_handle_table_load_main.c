/* The test harness for the rebuilt `object_handle_table_load` module.
 *
 * The harness holds the resource list pointer, the handle table pointer, the
 * handle table, and the `resource_get` stub.  The stub records its arguments
 * and returns the configured result.
 *
 * The harness runs three paths:
 *
 *     0  a non-zero result: the handle table pointer takes the resource, the
 *        first handle takes the free marker -1, and the call returns 0
 *     1  a zero result: the handle table pointer becomes zero, the table stays
 *        unchanged, and the call returns 0x12
 *     2  a second non-zero result with another resource list: the pointer and
 *        the free marker change, and the call returns 0
 *     D  the exit marker
 *
 * The string argument of the module literal is not readable in this minimal
 * image, so the stub records only the name pointer and the harness checks the
 * non-string arguments.
 */
#pragma off (check_stack)

int object_handle_table_load(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_0000801c;
int *DAT_00021540;

static char text[8];
static int table[4];

static void *resource_resource;
static char *resource_name;
static int resource_size;
static int resource_calls;
static void *resource_result;

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

void *resource_get(void *resource, char *name, int size)
{
    resource_resource = resource;
    resource_name = name;
    resource_size = size;
    resource_calls = resource_calls + 1;
    return resource_result;
}

static void reset(void)
{
    int index;

    DAT_0000801c = (void *)0x22222222;
    DAT_00021540 = (int *)0x33333333;
    for (index = 0; index < 4; index++) {
        table[index] = 0x5a5a5a5a;
    }
    resource_resource = 0;
    resource_name = 0;
    resource_size = 0;
    resource_calls = 0;
    resource_result = 0;
}

int main(void)
{
    int result;
    int ok;
    void *list;

    reset();
    resource_result = table;
    result = object_handle_table_load();
    ok = result == 0 && DAT_00021540 == table && table[0] == -1 &&
         resource_calls == 1 && resource_resource == DAT_0000801c &&
         resource_size == 0x17e80 && resource_name != 0;
    report(ok, '0');

    reset();
    table[0] = 0x12345678;
    resource_result = 0;
    result = object_handle_table_load();
    ok = result == 0x12 && DAT_00021540 == 0 && table[0] == 0x12345678 &&
         resource_calls == 1 && resource_resource == DAT_0000801c &&
         resource_size == 0x17e80 && resource_name != 0;
    report(ok, '1');

    reset();
    table[0] = 0x11111111;
    list = (void *)0x44444444;
    DAT_0000801c = list;
    resource_result = table;
    result = object_handle_table_load();
    ok = result == 0 && DAT_00021540 == table && table[0] == -1 &&
         resource_calls == 1 && resource_resource == list &&
         resource_size == 0x17e80;
    report(ok, '2');

    put('D');
    quit();
    return 0;
}
