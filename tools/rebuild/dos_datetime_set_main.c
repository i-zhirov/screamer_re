#pragma off (check_stack)

void dos_datetime_set(void);
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

char DAT_00000c08[40];
char DAT_00000c30[40];

static char text[8];
static int timer_calls;
static unsigned int timer_divisor;
static int fill_calls;
static int dpmi_calls;
static int int1a_calls;
static int int21_calls;
static unsigned char rtc_century;
static unsigned char rtc_year;
static unsigned char rtc_month;
static unsigned char rtc_day;
static unsigned char rtc_hour;
static unsigned char rtc_minute;
static unsigned char rtc_second;
static int date_invalid;
static int time_invalid;
static unsigned int dos_year;
static unsigned int dos_month;
static unsigned int dos_day;
static unsigned int dos_hour;
static unsigned int dos_minute;
static unsigned int dos_second;
static int date_error_calls;
static int time_error_calls;
static int error_calls;

void timer_set(unsigned int divisor)
{
    timer_calls = timer_calls + 1;
    timer_divisor = divisor;
}

void dpmi_fill_regs(struct dpmi_sregs *sregs, struct dpmi_regs *regs,
                    struct dpmi_regs *regs2, struct dpmi_sregs *sregs2)
{
    fill_calls = fill_calls + 1;
}

int dpmi_call(int intnum, void *regs_pointer, void *result_pointer,
              void *sregs_pointer)
{
    struct dpmi_regs *regs;

    regs = (struct dpmi_regs *)regs_pointer;
    dpmi_calls = dpmi_calls + 1;
    if (intnum == 0x1a) {
        int1a_calls = int1a_calls + 1;
        if ((regs->eax & 0xff00) == 0x0400) {
            regs->ecx = (rtc_century << 8) | rtc_year;
            regs->edx = (rtc_month << 8) | rtc_day;
        } else if ((regs->eax & 0xff00) == 0x0200) {
            regs->ecx = (rtc_hour << 8) | rtc_minute;
            regs->edx = rtc_second << 8;
        }
    } else if (intnum == 0x21) {
        int21_calls = int21_calls + 1;
        if ((regs->eax & 0xff00) == 0x2b00) {
            dos_year = regs->ecx & 0xffff;
            dos_month = (regs->edx >> 8) & 0xff;
            dos_day = regs->edx & 0xff;
            regs->eax = date_invalid ? 1 : 0;
        } else if ((regs->eax & 0xff00) == 0x2d00) {
            dos_hour = (regs->ecx >> 8) & 0xff;
            dos_minute = regs->ecx & 0xff;
            dos_second = (regs->edx >> 8) & 0xff;
            regs->eax = time_invalid ? 1 : 0;
        }
    }
    return 0;
}

void error_report(char *message)
{
    error_calls = error_calls + 1;
    if (message == DAT_00000c08) {
        date_error_calls = date_error_calls + 1;
    }
    if (message == DAT_00000c30) {
        time_error_calls = time_error_calls + 1;
    }
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

static void reset(void)
{
    timer_calls = 0;
    timer_divisor = 0xffffffff;
    fill_calls = 0;
    dpmi_calls = 0;
    int1a_calls = 0;
    int21_calls = 0;
    rtc_century = 0;
    rtc_year = 0;
    rtc_month = 0;
    rtc_day = 0;
    rtc_hour = 0;
    rtc_minute = 0;
    rtc_second = 0;
    date_invalid = 0;
    time_invalid = 0;
    dos_year = 0xffffffff;
    dos_month = 0xffffffff;
    dos_day = 0xffffffff;
    dos_hour = 0xffffffff;
    dos_minute = 0xffffffff;
    dos_second = 0xffffffff;
    date_error_calls = 0;
    time_error_calls = 0;
    error_calls = 0;
}

static void scenario(unsigned char century, unsigned char year,
                     unsigned char month, unsigned char day,
                     unsigned char hour, unsigned char minute,
                     unsigned char second)
{
    rtc_century = century;
    rtc_year = year;
    rtc_month = month;
    rtc_day = day;
    rtc_hour = hour;
    rtc_minute = minute;
    rtc_second = second;
}

static int check(unsigned int year, unsigned int month, unsigned int day,
                 unsigned int hour, unsigned int minute, unsigned int second,
                 int date_errors, int time_errors)
{
    return timer_calls == 1 && timer_divisor == 0
        && fill_calls == 4 && dpmi_calls == 4
        && int1a_calls == 2 && int21_calls == 2
        && dos_year == year && dos_month == month && dos_day == day
        && dos_hour == hour && dos_minute == minute && dos_second == second
        && date_error_calls == date_errors
        && time_error_calls == time_errors
        && error_calls == date_errors + time_errors;
}

int main(void)
{
    reset();
    scenario(0x20, 0x24, 0x06, 0x15, 0x13, 0x45, 0x30);
    dos_datetime_set();
    put(check(2024, 6, 15, 13, 45, 30, 0, 0) ? '0' : 'x');

    reset();
    scenario(0x19, 0x99, 0x12, 0x31, 0x23, 0x59, 0x59);
    dos_datetime_set();
    put(check(1999, 12, 31, 23, 59, 59, 0, 0) ? '1' : 'x');

    reset();
    scenario(0x20, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00);
    dos_datetime_set();
    put(check(2000, 1, 1, 0, 0, 0, 0, 0) ? '2' : 'x');

    reset();
    scenario(0x20, 0x24, 0x06, 0x15, 0x13, 0x45, 0x30);
    date_invalid = 1;
    dos_datetime_set();
    put(check(2024, 6, 15, 13, 45, 30, 1, 0) ? '3' : 'x');

    reset();
    scenario(0x20, 0x24, 0x06, 0x15, 0x13, 0x45, 0x30);
    time_invalid = 1;
    dos_datetime_set();
    put(check(2024, 6, 15, 13, 45, 30, 0, 1) ? '4' : 'x');

    reset();
    scenario(0x20, 0x24, 0x06, 0x15, 0x13, 0x45, 0x30);
    date_invalid = 1;
    time_invalid = 1;
    dos_datetime_set();
    put(check(2024, 6, 15, 13, 45, 30, 1, 1) ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
