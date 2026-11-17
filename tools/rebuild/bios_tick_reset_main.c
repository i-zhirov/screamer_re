/* The test harness for the rebuilt `bios_tick_reset` module.
 *
 * The harness holds the register block `DAT_0000cf18` and the segment set
 * `DAT_0000cf34`.  The `dpmi_fill_regs` stub records the call and checks the
 * segment pointer.  The `dpmi_call` stub records the call count, the call
 * order, the argument block, and the two INT 1Ah requests.
 *
 * The stubs script the BIOS timer service.  On AH=0 the stub returns the
 * scripted CX and DX values.  On AH=1 the stub records the requested CX and DX
 * values.  The harness runs three paths:
 *
 *     0  CX=0x4455, DX=0x6677: the function returns 0x6677 and sets the tick
 *        count to 0:0
 *     1  CX=0x1234, DX=0xffff: the function returns 0xffff
 *     2  CX=0x0000, DX=0x0000: the function returns 0
 *     D  the exit marker
 *
 * Every path checks the fill call count, the two INT 1Ah calls, the argument
 * block addresses, the first EAX value 0, the second EAX value 0x100, the saved
 * result, the zero set request, and the final register block values.
 */
#pragma off (check_stack)

unsigned int bios_tick_reset(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct dpmi_regs {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
    unsigned int esi;
    unsigned int edi;
    unsigned int flags;
};

struct dpmi_sregs {
    unsigned short es;
    unsigned short cs;
    unsigned short ss;
    unsigned short ds;
    unsigned short fs;
    unsigned short gs;
};

struct dpmi_regs DAT_0000cf18;
struct dpmi_sregs DAT_0000cf34;

static char text[8];

static int fill_calls;
static int dpmi_calls;
static int int1a_calls;
static int first_eax;
static int second_eax;
static unsigned int script_cx;
static unsigned int script_dx;
static unsigned int set_cx;
static unsigned int set_dx;
static int pointer_bad;
static int fill_pointer_bad;

void dpmi_fill_regs(struct dpmi_sregs *sregs)
{
    fill_calls = fill_calls + 1;
    if (sregs != &DAT_0000cf34) {
        fill_pointer_bad = 1;
    }
}

int dpmi_call(int intnum, struct dpmi_regs *regs,
              struct dpmi_regs *result, struct dpmi_sregs *sregs)
{
    int index;

    index = dpmi_calls;
    dpmi_calls = index + 1;
    if (intnum != 0x1a || regs != &DAT_0000cf18
        || result != &DAT_0000cf18 || sregs != &DAT_0000cf34) {
        pointer_bad = 1;
    }
    if (index == 0) {
        first_eax = regs->eax;
    } else if (index == 1) {
        second_eax = regs->eax;
    }
    if (intnum == 0x1a) {
        int1a_calls = int1a_calls + 1;
        if ((regs->eax & 0xff00) == 0) {
            regs->ecx = script_cx;
            regs->edx = script_dx;
        } else if ((regs->eax & 0xff00) == 0x100) {
            set_cx = regs->ecx & 0xffff;
            set_dx = regs->edx & 0xffff;
        }
    }
    return (int)regs->eax;
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

static int check(unsigned int value, unsigned int expected)
{
    return fill_calls == 1 && dpmi_calls == 2 && int1a_calls == 2
        && first_eax == 0 && second_eax == 0x100
        && value == expected
        && set_cx == 0 && set_dx == 0
        && DAT_0000cf18.eax == 0x100
        && DAT_0000cf18.ecx == 0
        && DAT_0000cf18.edx == 0
        && pointer_bad == 0 && fill_pointer_bad == 0;
}

static void reset(void)
{
    fill_calls = 0;
    dpmi_calls = 0;
    int1a_calls = 0;
    first_eax = -1;
    second_eax = -1;
    set_cx = 0xffffffffUL;
    set_dx = 0xffffffffUL;
    pointer_bad = 0;
    fill_pointer_bad = 0;
    DAT_0000cf18.eax = 0xaaaaaaaaUL;
    DAT_0000cf18.ebx = 0xbbbbbbbbUL;
    DAT_0000cf18.ecx = 0xccccccccUL;
    DAT_0000cf18.edx = 0xddddddddUL;
    DAT_0000cf18.esi = 0xeeeeeeeeUL;
    DAT_0000cf18.edi = 0xffffffffUL;
    DAT_0000cf18.flags = 0x11111111UL;
}

int main(void)
{
    unsigned int value;

    reset();
    script_cx = 0x4455UL;
    script_dx = 0x6677UL;
    value = bios_tick_reset();
    put(check(value, 0x6677UL) ? '0' : 'x');

    reset();
    script_cx = 0x1234UL;
    script_dx = 0xffffUL;
    value = bios_tick_reset();
    put(check(value, 0xffffUL) ? '1' : 'x');

    reset();
    script_cx = 0x0000UL;
    script_dx = 0x0000UL;
    value = bios_tick_reset();
    put(check(value, 0x0000UL) ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
