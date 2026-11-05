/* The test harness for the rebuilt `filter_update` module.
 *
 * The harness holds the filter table and the filter value.  The module reads
 * the table entry, applies the one-pole filter, and writes the result back to
 * the value.
 *
 *     V  the value holds `(1.0 - 0.5) * 0.25 + 0.5`
 *     D  the exit marker
 *
 * The table is a stack-local array, because the pointer value that the module
 * dereferences must be a real address: a global pointer initializer holds a
 * relocated immediate that this minimal build does not patch at run time, so
 * a static table address would read the wrong memory.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void filter_update(float *p, float f, int i);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
float *DAT_0010cf08;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int main(void)
{
    float local_table[4];
    float value;

    local_table[0] = 1.0f;
    local_table[1] = 0.5f;
    local_table[2] = 0.25f;
    local_table[3] = 0.125f;
    DAT_0010cf08 = local_table;
    value = 1.0f;
    filter_update(&value, 0.5f, 2);
    put(value > 0.62f && value < 0.63f ? 'V' : 'v');
    put('D');
    quit();
    return 0;
}
