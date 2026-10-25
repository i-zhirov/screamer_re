/* Rebuild the HMI driver register write sequence without the C run-time.
 *
 * The original driver HMIMDRV.386 writes the OPL registers in the order register
 * then data: mov dx, 0x20; mov al, 0x20; out dx, al; mov eax, [0xF1];
 * mov dx, 0x21; out dx, al (the dump at 0x14157).  This driver holds the
 * eight pairs of the first DRUM.BNK record and writes them in that order.  The
 * pairs are immediates, so the program needs no data segment.
 */

#pragma off (check_stack)

void out_port(unsigned short port, unsigned char value);
#pragma aux out_port = "out dx, al" parm [dx] [al] modify exact [];

void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];

void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char line[16];

static char digit(unsigned char v)
{
    v = v & 15;
    if (v < 10) {
        return (char)('0' + v);
    }
    return (char)('A' + v - 10);
}

static void show_pair(unsigned char reg, unsigned char value)
{
    line[0] = 'W'; line[1] = ' '; line[2] = digit(reg >> 4); line[3] = digit(reg & 15);
    line[4] = ' '; line[5] = digit(value >> 4); line[6] = digit(value & 15);
    line[7] = 13; line[8] = 10; line[9] = '$';
    print(line);
}

static void pair(unsigned char reg, unsigned char value)
{
    show_pair(reg, value);
    out_port(0x388, reg);
    out_port(0x389, value);
}

int main(void)
{
    print("REBUILT$");
    pair(0x20, 0x0F);
    pair(0x40, 0x3F);
    pair(0x60, 0x01);
    pair(0x80, 0x3E);
    pair(0xE0, 0x0F);
    pair(0x23, 0x3F);
    pair(0x43, 0x01);
    pair(0xC0, 0x01);
    print("DONE$");
    quit();
    return 0;
}
