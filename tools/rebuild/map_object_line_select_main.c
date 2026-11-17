/* The test harness for the rebuilt `map_object_line_select` module.
 *
 * The harness holds the 0x50 map object records at `DAT_001db78`, stride
 * 0x64.  Each record holds the key at `+0x04` and the animation flag at
 * `+0x0c`.  The function sets the flag to zero on the matching records and to
 * one on the other records.
 *
 * The harness runs these paths:
 *
 *     0  one matching record among the flags 1: the selected flag reaches zero
 *        and a non-matching flag 0 returns to one
 *     1  a code that no record holds: every flag reaches one from zero
 *     2  three matching records: each reaches zero and the other flags stay
 *        one
 *     3  a mix of flags and keys: a matching flag 0 stays zero, a non-matching
 *        flag 1 stays one, a non-matching flag 0 reaches one, and a matching
 *        flag 1 reaches zero
 *     D  the exit marker
 */
#pragma off (check_stack)

#define RECORD_SIZE 0x64
#define RECORD_COUNT 0x50
#define KEY_OFFSET 0x04
#define FLAG_OFFSET 0x0c

void map_object_line_select(int code);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_001db78[RECORD_SIZE * RECORD_COUNT];
static char text[8];

static int *key_at(int index)
{
    return (int *)(DAT_001db78 + index * RECORD_SIZE + KEY_OFFSET);
}

static int *flag_at(int index)
{
    return (int *)(DAT_001db78 + index * RECORD_SIZE + FLAG_OFFSET);
}

static void set_keys(void)
{
    int index;

    for (index = 0; index < RECORD_COUNT; index++)
        *key_at(index) = 0x200 + index;
}

static void set_flags(int value)
{
    int index;

    for (index = 0; index < RECORD_COUNT; index++)
        *flag_at(index) = value;
}

static int flag_count(int value)
{
    int index;
    int count;

    count = 0;
    for (index = 0; index < RECORD_COUNT; index++) {
        if (*flag_at(index) == value)
            count++;
    }
    return count;
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
    int ok;

    set_keys();
    set_flags(1);
    *flag_at(10) = 0;
    map_object_line_select(*key_at(5));
    ok = *flag_at(5) == 0
        && *flag_at(10) == 1
        && flag_count(0) == 1
        && flag_count(1) == RECORD_COUNT - 1;
    put(ok ? '0' : 'x');

    set_flags(0);
    map_object_line_select(0x9999);
    ok = flag_count(1) == RECORD_COUNT && flag_count(0) == 0;
    put(ok ? '1' : 'x');

    set_keys();
    *key_at(3) = 0x2223;
    *key_at(7) = 0x2223;
    *key_at(9) = 0x2223;
    set_flags(0);
    *flag_at(4) = 1;
    *flag_at(3) = 1;
    map_object_line_select(0x2223);
    ok = *flag_at(3) == 0
        && *flag_at(7) == 0
        && *flag_at(9) == 0
        && *flag_at(4) == 1
        && flag_count(0) == 3
        && flag_count(1) == RECORD_COUNT - 3;
    put(ok ? '2' : 'x');

    set_keys();
    *key_at(2) = 0x3333;
    *key_at(8) = 0x3333;
    set_flags(1);
    *flag_at(2) = 0;
    *flag_at(6) = 0;
    *flag_at(8) = 1;
    map_object_line_select(0x3333);
    ok = *flag_at(2) == 0
        && *flag_at(4) == 1
        && *flag_at(6) == 1
        && *flag_at(8) == 0
        && flag_count(0) == 2;
    put(ok ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
