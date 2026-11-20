/* The test harness for the rebuilt `dpmi_alloc_dos` module.
 *
 * The harness calls the real DPMI INT 31h path under DOSBox-X.  It runs these
 * paths:
 *
 *     0  the size 0x100: status 0, a linear address below 1 MB, and the
 *        16-byte alignment
 *     1  the size 0: status 0 and a non-zero one-paragraph block address
 *     2  the size 0xFFFE0, which requests 0xffff paragraphs: status 1 and an
 *        unchanged output address
 *     D  the exit marker
 *
 * The output address comes through the second argument, because C cannot
 * return a second value in `edi`.  A sentinel starts the address, so path 2
 * proves that the failure branch writes no address.
 */
#pragma off (check_stack)

unsigned int dpmi_alloc_dos(unsigned int size, unsigned int *linear);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

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

static int block_ok(unsigned int status, unsigned int linear)
{
    if (status != 0) {
        return 0;
    }
    if (linear == 0) {
        return 0;
    }
    if ((linear & 0xf) != 0) {
        return 0;
    }
    if (linear >= 0x100000) {
        return 0;
    }
    return 1;
}

int main(void)
{
    unsigned int linear;
    unsigned int status;

    linear = 0xdeadbeef;
    status = dpmi_alloc_dos(0x100, &linear);
    put(block_ok(status, linear) ? '0' : 'x');

    linear = 0xdeadbeef;
    status = dpmi_alloc_dos(0, &linear);
    put(block_ok(status, linear) ? '1' : 'x');

    linear = 0xdeadbeef;
    status = dpmi_alloc_dos(0xfffe0, &linear);
    put(status == 1 && linear == 0xdeadbeef ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
