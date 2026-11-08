/* The rebuilt `map_object_place` module.
 *
 * The original sits at linear 0x2b044 in STARTH.EXE and holds 222 instructions
 * and 708 bytes up to the `ret` at 0x2b307.  The next function starts at
 * 0x2b308, so the record keeps the 708-byte extent and sets no function size.
 *
 * The only direct caller is at 0x29a88 in the race state update at 0x29918.
 * For each line of the selected menu the caller passes one line descriptor and
 * one string.  The `map_object_update` module at 0x2b9cc reads the records that
 * this function writes, and the placement runs before the update, so the
 * module records the name `map_object_place`.
 *
 * The function computes the start position of the line.  A centered line
 * starts at `(length - 1) * 320`, and the other lines start at
 * `0x1200 - field8 * 640`.  The vertical position is `0x820 - field4 * 384`.
 *
 * The function walks the string until the first code outside 0x20 through
 * 0x7f.  For each code it takes the first free record of the 0x50 records at
 * `DAT_001db78`, each 0x64 bytes long.  It stores the code minus 0x20 at
 * `+0x08`, the line id at `+0x04`, the target x at `+0x40`, the target y at
 * `+0x44`, the target z 0 at `+0x48`, the start x and y at `+0x34` and `+0x38`
 * or the target x and y, the start z at `+0x3c`, the state flag at `+0x0c`,
 * the angle pi or 0 at `+0x10`, the two amplitudes at `+0x4c` and `+0x50`, the
 * progress 1.0 at `+0x58`, the direction 1.0 at `+0x5c`, the angle rate 1.0 at
 * `+0x1c`, the zero fields, and the timer at `+0x60`.  The timer rises by 0.01
 * for each placed code.
 *
 * The function then rotates the line origin by the angle at `+0x28` for the
 * next code.  The new x is `y * sin + x * cos` and the new y is
 * `y * cos - x * sin`, truncated toward zero.
 *
 * The original calls the cosine helper at 0x4da66, the sine helper at 0x4da72,
 * and the truncate helper at 0x4d9f4.  This harness links no math library, so
 * the local FPU stub supplies `cos` and `sin` from the hardware and makes
 * `__CHP` truncate toward zero.  The timer step arrives through a global,
 * because an FPU memory operand of a literal reads the constant pool at a low
 * address in this toolchain.
 *
 * The original has the EBP frame, so the module builds with `-of+`.
 */
extern double cos(double value);
extern double sin(double value);

extern char DAT_001db78[];
extern float map_object_timer_step;

struct map_line {
    int line;
    int field4;
    int field8;
    int centered;
    int flag10;
    int start_x;
    int start_y;
    int start_z;
    int amplitude_x;
    int amplitude_y;
    float angle;
    float timer;
};

void map_object_place(struct map_line *line, char *text)
{
    char *record;
    int x;
    int y;
    int i;
    int j;
    int length;
    int start_x;
    int start_y;
    float cos_value;
    float sin_value;

    if (line->centered != 0) {
        length = 0;
        while (text[length] != '\0') {
            length++;
        }
        x = 0x1200 - (int)(
            (unsigned int)(0x2400 - 640 * (length - 1)) >> 1);
    } else {
        x = 0x1200 - 640 * line->field8;
    }
    y = 0x820 - 384 * line->field4;

    i = 0;
    j = 0;
    for (;;) {
        unsigned char code;

        code = (unsigned char)text[i];
        if (code < 0x20 || code >= 0x80) {
            break;
        }
        for (;;) {
            if (j >= 0x50) {
                break;
            }
            record = DAT_001db78 + j * 0x64;
            if (*(int *)(record + 8) == -1) {
                break;
            }
            j++;
        }
        if (j >= 0x50) {
            continue;
        }

        *(int *)(record + 8) = (int)code - 0x20;
        *(int *)(record + 4) = line->line;
        *(int *)(record + 0x40) = x;
        if (line->start_x != 0) {
            *(int *)(record + 0x34) = line->start_x;
        } else {
            *(int *)(record + 0x34) = x;
        }
        x -= 0x280;
        *(int *)(record + 0x44) = y;
        if (line->start_y != 0) {
            *(int *)(record + 0x38) = line->start_y;
        } else {
            *(int *)(record + 0x38) = y;
        }
        *(int *)(record + 0x48) = 0;
        *(int *)(record + 0x3c) = line->start_z;
        if (line->flag10 != 0) {
            *(int *)(record + 0x10) = 0x40490fdb;
        } else {
            *(int *)(record + 0x10) = 0;
        }
        *(int *)(record + 0x14) = 0;
        *(int *)(record + 0x0c) = line->flag10;
        *(int *)(record + 0x18) = 0;
        *(int *)(record + 0x4c) = line->amplitude_x;
        *(int *)(record + 0x50) = line->amplitude_y;
        *(float *)(record + 0x58) = 1.0f;
        *(float *)(record + 0x5c) = 1.0f;
        *(float *)(record + 0x1c) = 1.0f;
        *(int *)(record + 0x28) = 0;
        *(int *)(record + 0x20) = 0;
        *(float *)(record + 0x2c) = 1.0f;
        *(int *)(record + 0x30) = 0;
        *(int *)(record + 0x24) = 0;
        *(float *)(record + 0x60) = line->timer;
        line->timer = line->timer + map_object_timer_step;

        j++;
        i++;

        start_x = line->start_x;
        start_y = line->start_y;
        cos_value = (float)cos((double)line->angle);
        sin_value = (float)sin((double)line->angle);
        line->start_x = (int)((double)start_y * (double)sin_value
            + (double)start_x * (double)cos_value);
        line->start_y = (int)((double)start_y * (double)cos_value
            - (double)start_x * (double)sin_value);
    }
}
