/* The test harness for the rebuilt `keyboard_hook` module.
 *
 * The harness holds the register block, the segment set, the saved vector,
 * and the controller state.  The `inportb` stub reports the controller status
 * and the acknowledge sequence.  The `outportb` stub records every byte that
 * the module writes.  The `dpmi_call` stub answers the get-vector call with
 * the saved vector and records the set-vector call.
 *
 * The harness runs five paths:
 *
 *     0  the install path accepts the first acknowledge and installs CS:0xe4
 *     1  the first acknowledge is wrong and the second attempt succeeds
 *     2  every acknowledge is wrong and the module installs after four tries
 *     3  the uninstall path restores the saved vector and touches no port
 *     4  the install path clears only the three lock bits of the BIOS flags
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#include <i86.h>

#pragma off (check_stack)

void keyboard_hook(int value);
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

unsigned short DAT_0000cf0c;
unsigned int DAT_0000cf10;
struct dpmi_regs DAT_0000cf18;
struct dpmi_sregs DAT_0000cf34;

static char text[8];
static unsigned char writes[16];
static int write_count;
static unsigned char acknowledges[8];
static int acknowledge_count;
static int acknowledge_index;
static int controller_status;
static int dpmi_calls;
static int get_calls;
static int set_calls;
static unsigned int last_get_eax;
static unsigned int last_set_eax;
static unsigned int last_set_edx;
static unsigned int last_set_es;
static unsigned int last_set_ds;
static unsigned int saved_segment;
static unsigned int saved_offset;

unsigned short harness_cs(void);
#pragma aux harness_cs = "mov ax, cs" value [ax] modify exact [ax];

unsigned char inportb(unsigned short port)
{
    if (port == 0x64) {
        return (unsigned char)controller_status;
    }
    if (port == 0x60) {
        if (acknowledge_index < acknowledge_count) {
            return acknowledges[acknowledge_index++];
        }
        return 0xfa;
    }
    return 0;
}

void outportb(unsigned short port, unsigned char value)
{
    if (write_count < 16) {
        writes[write_count] = value;
    }
    write_count = write_count + 1;
}

int dpmi_call(int intnum, void *regs_pointer, void *result_pointer,
              void *sregs_pointer)
{
    struct dpmi_regs *regs;
    struct dpmi_sregs *sregs;

    regs = (struct dpmi_regs *)regs_pointer;
    sregs = (struct dpmi_sregs *)sregs_pointer;
    dpmi_calls = dpmi_calls + 1;
    if (regs->eax == 0x3509) {
        get_calls = get_calls + 1;
        last_get_eax = regs->eax;
        regs->ebx = saved_offset;
        sregs->es = (unsigned short)saved_segment;
    } else if (regs->eax == 0x2509) {
        set_calls = set_calls + 1;
        last_set_eax = regs->eax;
        last_set_edx = regs->edx;
        last_set_es = sregs->es;
        last_set_ds = sregs->ds;
    }
    return 0;
}

static unsigned char bios_read(void)
{
    unsigned char __far *flags;

    flags = (unsigned char __far *)MK_FP(0x40, 0x17);
    return *flags;
}

static void bios_write(unsigned char value)
{
    unsigned char __far *flags;

    flags = (unsigned char __far *)MK_FP(0x40, 0x17);
    *flags = value;
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

    write_count = 0;
    acknowledge_count = 0;
    acknowledge_index = 0;
    controller_status = 1;
    dpmi_calls = 0;
    get_calls = 0;
    set_calls = 0;
    last_get_eax = 0;
    last_set_eax = 0;
    last_set_edx = 0;
    last_set_es = 0xffff;
    last_set_ds = 0xffff;
    saved_segment = 0x1234;
    saved_offset = 0x5678;
    DAT_0000cf0c = 0;
    DAT_0000cf10 = 0;
    DAT_0000cf18.eax = 0;
    DAT_0000cf18.ebx = 0;
    DAT_0000cf18.ecx = 0;
    DAT_0000cf18.edx = 0;
    DAT_0000cf18.esi = 0;
    DAT_0000cf18.edi = 0;
    DAT_0000cf18.flags = 0;
    DAT_0000cf34.es = 0;
    DAT_0000cf34.cs = 0;
    DAT_0000cf34.ss = 0;
    DAT_0000cf34.ds = 0;
    DAT_0000cf34.fs = 0;
    DAT_0000cf34.gs = 0;
    for (index = 0; index < 16; ++index) {
        writes[index] = 0;
    }
    for (index = 0; index < 8; ++index) {
        acknowledges[index] = 0xfa;
    }
}

static int first_write_is(unsigned char value)
{
    return write_count >= 1 && writes[0] == value;
}

int main(void)
{
    unsigned char original_flags;

    reset();
    acknowledges[0] = 0xfa;
    acknowledge_count = 1;
    keyboard_hook(1);
    put(write_count == 2 && first_write_is(0xed) && writes[1] == 0
        && acknowledge_index == 1
        && dpmi_calls == 2 && get_calls == 1 && set_calls == 1
        && last_get_eax == 0x3509 && last_set_eax == 0x2509
        && last_set_edx == 0xe4 && last_set_es == 0
        && last_set_ds == harness_cs()
        && DAT_0000cf0c == 0x1234 && DAT_0000cf10 == 0x5678 ? '0' : 'x');

    reset();
    acknowledges[0] = 0x00;
    acknowledges[1] = 0xfa;
    acknowledge_count = 2;
    keyboard_hook(1);
    put(write_count == 3 && first_write_is(0xed) && writes[1] == 0xed
        && writes[2] == 0 && acknowledge_index == 2
        && dpmi_calls == 2 && get_calls == 1 && set_calls == 1 ? '1' : 'x');

    reset();
    acknowledges[0] = 0x00;
    acknowledges[1] = 0x00;
    acknowledges[2] = 0x00;
    acknowledges[3] = 0x00;
    acknowledge_count = 4;
    keyboard_hook(1);
    put(write_count == 5 && first_write_is(0xed) && writes[1] == 0xed
        && writes[2] == 0xed && writes[3] == 0xed && writes[4] == 0
        && acknowledge_index == 4
        && dpmi_calls == 2 && get_calls == 1 && set_calls == 1 ? '2' : 'x');

    reset();
    DAT_0000cf0c = 0x1234;
    DAT_0000cf10 = 0x5678;
    keyboard_hook(0);
    put(write_count == 0 && dpmi_calls == 1 && get_calls == 0 && set_calls == 1
        && last_set_eax == 0x2509 && last_set_edx == 0x5678
        && last_set_es == 0 && last_set_ds == 0x1234 ? '3' : 'x');

    reset();
    original_flags = bios_read();
    bios_write(0xf7);
    acknowledges[0] = 0xfa;
    acknowledge_count = 1;
    keyboard_hook(1);
    put(bios_read() == 0x87 ? '4' : 'x');
    bios_write(original_flags);

    put('D');
    quit();
    return 0;
}
