/* The test harness for the rebuilt `array_sort` module.
 *
 * The harness holds one work buffer and the input and expected arrays.  It
 * puts a guard dword just after each input, so a write past the element count
 * fails the path.
 *
 * The paths:
 *
 *     0  an ascending array stays in order
 *     1  a descending array becomes ascending
 *     2  an array with negative values becomes ascending
 *     3  an array with duplicate values keeps every element
 *     4  a single element stays in place
 *     5  a pair in the wrong order becomes ascending
 *     6  an empty array is unchanged
 *     7  a larger mixed array becomes ascending
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#define GUARD 0x5a5a5a5a
#define LIMIT 16

void array_sort(int *values, int count);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static int buffer[LIMIT + 1];

static int ascending[] = {1, 2, 3, 4, 5};
static int descending[] = {5, 4, 3, 2, 1};
static int mixed_in[] = {9, -1, 0, 5, -3};
static int mixed_out[] = {-3, -1, 0, 5, 9};
static int duplicates_in[] = {2, 2, 1, 2, 1};
static int duplicates_out[] = {1, 1, 2, 2, 2};
static int single[] = {7};
static int pair_in[] = {2, 1};
static int pair_out[] = {1, 2};
static int larger_in[] = {10, 3, 10, 3, 5, 1, 7, 2, 8, 6, 4, 9};
static int larger_out[] = {1, 2, 3, 3, 4, 5, 6, 7, 8, 9, 10, 10};

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int path(int *input, int *expected, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        buffer[index] = input[index];
    }
    buffer[count] = GUARD;
    array_sort(buffer, count);
    if (buffer[count] != GUARD) {
        return 0;
    }
    for (index = 0; index < count; index++) {
        if (buffer[index] != expected[index]) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    put(path(ascending, ascending, 5) ? '0' : 'x');
    put(path(descending, ascending, 5) ? '1' : 'x');
    put(path(mixed_in, mixed_out, 5) ? '2' : 'x');
    put(path(duplicates_in, duplicates_out, 5) ? '3' : 'x');
    put(path(single, single, 1) ? '4' : 'x');
    put(path(pair_in, pair_out, 2) ? '5' : 'x');
    put(path(ascending, ascending, 0) ? '6' : 'x');
    put(path(larger_in, larger_out, 12) ? '7' : 'x');
    put('D');
    quit();
    return 0;
}
