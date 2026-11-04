/* The test harness for the rebuilt `car_setup` module.
 *
 * The harness holds the slot counter, the limit, the reserved slot, the player
 * count, and the 0x590-byte car record.  The `object_load_named` stub records
 * its three arguments and returns the configured result, so the harness checks
 * the table, the name offset, and the destination of the two loads.
 *
 * The harness runs seven paths:
 *
 *     0  the normal path: the index 3 and the slot 5 select the model at
 *        `0xd25c + 5 * 0x40`
 *     1  the counter 4 at the limit 4 resets to 0, the reserved slot 0
 *        advances the counter to 1, and the model at `0xd25c + 0x40` runs
 *     2  the counter 0 advances past the reserved slot 0 to 1 and then wraps
 *        to 0 at the limit 1, so the model at `0xd25c` runs
 *     3  the player count 3 forces the model index 0xc, so the model at
 *        `0xd25c + 0xc * 0x40` runs
 *     4  the first load returns 5, so the function returns 5, the model index
 *        keeps its sentinel, and the second load does not run
 *     5  the second load returns 7, so the function returns 7 and the model
 *        index 3 stays set
 *     6  the load flag 0 returns 0, runs no load, and leaves the counter 5
 *        unchanged at the limit 3
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

int car_setup(void *object, int index, int load);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0000f8f0;
int DAT_0000d6dc;
int DAT_0011d2a0;
int DAT_0011d2b4;

static unsigned char object[0x590];
static char text[8];

static int load_calls;
static void *load_table[4];
static char *load_name[4];
static int *load_slot[4];
static int load_result[4];

int object_load_named(void *table, char *name, int *slot)
{
    int index;

    index = load_calls;
    if (index < 4) {
        load_table[index] = table;
        load_name[index] = name;
        load_slot[index] = slot;
    }
    load_calls = index + 1;
    if (index < 4) {
        return load_result[index];
    }
    return 0;
}

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x590; ++index) {
        object[index] = 0;
    }
    DAT_0000f8f0 = 0;
    DAT_0000d6dc = 10;
    DAT_0011d2a0 = 99;
    DAT_0011d2b4 = 1;
    load_calls = 0;
    for (index = 0; index < 4; ++index) {
        load_table[index] = 0;
        load_name[index] = 0;
        load_slot[index] = 0;
        load_result[index] = 0;
    }
}

static int first_load(int index)
{
    return load_table[0] == (void *)0x6530
        && load_name[0] == (char *)0x5dc
        && load_slot[0] == (int *)(object + 4)
        && *(int *)object == index;
}

static int second_load(int choice)
{
    return load_table[1] == (void *)0x6530
        && load_name[1] == (char *)(0xd25c + (choice << 6))
        && load_slot[1] == (int *)(object + 0x4dc)
        && *(int *)(object + 0x2e0) == choice;
}

int main(void)
{
    int result;

    reset();
    DAT_0000f8f0 = 5;
    result = car_setup(object, 3, 1);
    put(result == 0 && load_calls == 2 && first_load(3) && second_load(5)
        ? '0' : 'x');

    reset();
    DAT_0000f8f0 = 4;
    DAT_0000d6dc = 4;
    DAT_0011d2a0 = 0;
    result = car_setup(object, 1, 1);
    put(result == 0 && load_calls == 2 && first_load(1) && second_load(1)
        ? '1' : 'x');

    reset();
    DAT_0000d6dc = 1;
    DAT_0011d2a0 = 0;
    result = car_setup(object, 2, 1);
    put(result == 0 && load_calls == 2 && first_load(2) && second_load(0)
        ? '2' : 'x');

    reset();
    DAT_0000f8f0 = 2;
    DAT_0011d2b4 = 3;
    result = car_setup(object, 4, 1);
    put(result == 0 && load_calls == 2 && first_load(4) && second_load(0xc)
        ? '3' : 'x');

    reset();
    DAT_0000f8f0 = 5;
    *(int *)(object + 0x2e0) = -1;
    load_result[0] = 5;
    result = car_setup(object, 5, 1);
    put(result == 5 && load_calls == 1 && first_load(5)
        && *(int *)(object + 0x2e0) == -1 ? '4' : 'x');

    reset();
    DAT_0000f8f0 = 3;
    load_result[1] = 7;
    result = car_setup(object, 6, 1);
    put(result == 7 && load_calls == 2 && first_load(6) && second_load(3)
        ? '5' : 'x');

    reset();
    DAT_0000f8f0 = 5;
    DAT_0000d6dc = 3;
    DAT_0011d2a0 = 5;
    result = car_setup(object, 7, 0);
    put(result == 0 && load_calls == 0 && *(int *)object == 7
        && DAT_0000f8f0 == 5 ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
