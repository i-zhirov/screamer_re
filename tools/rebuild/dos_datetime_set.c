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

extern void timer_set(unsigned int divisor);
extern void dpmi_fill_regs(struct dpmi_sregs *sregs, struct dpmi_regs *regs,
                           struct dpmi_regs *regs2, struct dpmi_sregs *sregs2);
extern int dpmi_call(int intnum, struct dpmi_regs *regs,
                     struct dpmi_regs *result, struct dpmi_sregs *sregs);
extern void error_report(char *text);
extern char DAT_00000c08[];
extern char DAT_00000c30[];

#define BCD(value) (((((value) >> 4) & 0x0f) * 10) + ((value) & 0x0f))

void dos_datetime_set(void)
{
    struct dpmi_regs regs;
    struct dpmi_sregs sregs;
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int hour;
    unsigned int minute;
    unsigned int second;

    timer_set(0);
    regs.eax = 0x400;
    dpmi_fill_regs(&sregs, &regs, &regs, &sregs);
    dpmi_call(0x1a, &regs, &regs, &sregs);

    month = BCD((regs.edx >> 8) & 0xff);
    day = BCD(regs.edx & 0xff);
    year = BCD((regs.ecx >> 8) & 0xff) * 100 + BCD(regs.ecx & 0xff);

    regs.eax = 0x200;
    dpmi_fill_regs(&sregs, &regs, &regs, &sregs);
    dpmi_call(0x1a, &regs, &regs, &sregs);

    hour = BCD((regs.ecx >> 8) & 0xff);
    minute = BCD(regs.ecx & 0xff);
    second = BCD((regs.edx >> 8) & 0xff);

    regs.eax = 0x2b00;
    regs.ecx = year;
    regs.edx = (month << 8) | day;
    dpmi_fill_regs(&sregs, &regs, &regs, &sregs);
    dpmi_call(0x21, &regs, &regs, &sregs);
    if ((regs.eax & 0xff) != 0) {
        error_report(DAT_00000c08);
    }

    regs.eax = 0x2d00;
    regs.ecx = (hour << 8) | minute;
    regs.edx = second << 8;
    dpmi_fill_regs(&sregs, &regs, &regs, &sregs);
    dpmi_call(0x21, &regs, &regs, &sregs);
    if ((regs.eax & 0xff) != 0) {
        error_report(DAT_00000c30);
    }
}
