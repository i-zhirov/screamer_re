/* The test harness for the rebuilt `state_cells_set` module.
 *
 * The harness holds the ten data cells that the function writes.
 *
 * The paths:
 *
 *     0  the default value 0xffff54e0: the eight cells hold the value, the
 *        negated cell holds 0xab20, and the biased cell holds 0xffff5460
 *     1  the value 0x12345678: the eight cells hold the value, the negated
 *        cell holds 0xedcba988, and the biased cell holds 0x123455f8
 *     2  the value 0: the eight cells hold zero, the negated cell holds zero,
 *        and the biased cell holds -0x80
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void state_cells_set(int value);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00073bf7;
int DAT_00073c05;
int DAT_00073c13;
int DAT_00073c21;
int DAT_00073c68;
int DAT_00073c76;
int DAT_00073c84;
int DAT_000c7cf0;
int DAT_000c7cf4;
int DAT_00072bf0;

static char text[8];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static int same_all(int value)
{
    return DAT_00073bf7 == value
        && DAT_00073c05 == value
        && DAT_00073c13 == value
        && DAT_00073c21 == value
        && DAT_00073c68 == value
        && DAT_00073c76 == value
        && DAT_00073c84 == value
        && DAT_000c7cf0 == value;
}

static void check(int number, int ok)
{
    put(ok ? (char)('0' + number) : 'x');
}

int main(void)
{
    state_cells_set((int)0xffff54e0);
    check(0, same_all((int)0xffff54e0)
        && DAT_000c7cf4 == 0xab20
        && DAT_00072bf0 == (int)0xffff5460);

    state_cells_set(0x12345678);
    check(1, same_all(0x12345678)
        && DAT_000c7cf4 == (int)0xedcba988
        && DAT_00072bf0 == 0x123455f8);

    state_cells_set(0);
    check(2, same_all(0)
        && DAT_000c7cf4 == 0
        && DAT_00072bf0 == -0x80);

    put('D');
    quit();
    return 0;
}
