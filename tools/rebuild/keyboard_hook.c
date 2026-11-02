/* The rebuilt `keyboard_hook` module.
 *
 * The original sits at linear 0x14e20 in STARTH.EXE.  The body holds 100
 * instructions and 334 bytes up to the early-exit block at 0x14f6e.  The
 * merged store record keeps the 408-byte extent that includes the trailing
 * NOP at 0x14fb7.
 *
 * The function has two paths.  With a non-zero argument it sends the keyboard
 * controller command 0xed, waits for the 0xfa acknowledge, sends the LED data
 * 0, clears the lock bits of the BIOS keyboard flags at 0040:0017, reads the
 * real-mode interrupt 9 vector with INT 21h AX=3509h, and installs the handler
 * at CS:0xe4 with INT 21h AX=2509h.  With a zero argument it restores the
 * saved real-mode interrupt 9 vector.
 *
 * The two interrupt calls go through `dpmi_call` at 0x4bf4b with the register
 * block at 0xcf18 and the segment set at 0xcf34.  The original reads the
 * keyboard controller with `in` and writes it with `out`.  The rebuild calls
 * `inportb` and `outportb`, so the test harness can simulate the controller
 * and prove the acknowledge and retry paths.  The interface is the set of
 * helper calls that the original inlines.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
#include <i86.h>

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

extern int dpmi_call(int intnum, void *regs, void *result, void *sregs);
extern unsigned char inportb(unsigned short port);
extern void outportb(unsigned short port, unsigned char value);

extern unsigned short DAT_0000cf0c;
extern unsigned int DAT_0000cf10;
extern struct dpmi_regs DAT_0000cf18;
extern struct dpmi_sregs DAT_0000cf34;

unsigned short keyboard_hook_cs(void);
#pragma aux keyboard_hook_cs = "mov ax, cs" value [ax] modify exact [ax];

void keyboard_hook(int value)
{
    int retries;
    int timeout;
    unsigned char status;
    unsigned short code_segment;

    if (value == 0) {
        DAT_0000cf18.eax = 0x2509;
        DAT_0000cf18.edx = DAT_0000cf10;
        DAT_0000cf34.es = 0;
        DAT_0000cf34.ds = DAT_0000cf0c;
        dpmi_call(0x21, &DAT_0000cf18, &DAT_0000cf18, &DAT_0000cf34);
        return;
    }

    retries = 3;
    timeout = 0xffff;
    do {
        while (timeout > 0) {
            status = inportb(0x64);
            if ((status & 2) == 0) {
                break;
            }
            timeout = timeout - 1;
        }
        outportb(0x60, 0xed);
        while (timeout > 0) {
            status = inportb(0x64);
            if ((status & 1) != 0) {
                break;
            }
            timeout = timeout - 1;
        }
        status = inportb(0x60);
        retries = retries - 1;
    } while (retries != -1 && status != 0xfa);
    outportb(0x60, 0);

    *(unsigned char __far *)MK_FP(0x40, 0x17) &= 0x8f;

    DAT_0000cf18.eax = 0x3509;
    DAT_0000cf34.es = 0;
    DAT_0000cf34.ds = 0;
    dpmi_call(0x21, &DAT_0000cf18, &DAT_0000cf18, &DAT_0000cf34);

    DAT_0000cf0c = DAT_0000cf34.es;
    DAT_0000cf18.eax = 0x2509;
    DAT_0000cf10 = DAT_0000cf18.ebx;
    code_segment = keyboard_hook_cs();
    DAT_0000cf18.edx = 0xe4;
    DAT_0000cf34.es = 0;
    DAT_0000cf34.ds = code_segment;
    dpmi_call(0x21, &DAT_0000cf18, &DAT_0000cf18, &DAT_0000cf34);
}
